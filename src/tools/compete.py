#!/usr/bin/python

import fcntl
import os
import subprocess
import sys
import time
from threading import Thread

valid_commands = ['go', 'usermove', 'move', 'new', 'variant']


class EngineInfo(object):
  """Contains name and binary path of an engine."""

  def __init__(self, name, binary):
    self.name = name
    self.binary = binary

  def Name(self):
    return self.name

  def Binary(self):
    return self.binary


class Game(object):
  """Plays a game between two engines.

  Begins a match between two engines for given number of seconds. At the end
  of the match, the result is returned. The Game object takes care of starting
  and stopping the engines. The first engine always plays white.
  """

  def __init__(self, engine1_info, engine2_info, seconds):
    """Constructor.

    Args:
      engine1_info: The first EngineInfo object. This engine plays white.
      engine2_info: The second EngineInfo object. This engine plays black.
      seconds: The duration of the game.
    """
    self.engine1_info = engine1_info
    self.engine2_info = engine2_info
    self.seconds = seconds
    self.is_game_end = False
    self.total_moves = 0
    self.winner = 'No result'

  def _TimeNowCentis(self):
    """Current time in centi seconds."""
    return int(time.time() * 100)

  def _StartEngine(self, engine_bin, opp_name):
    """Starts a Chess engine and initializes it.

    Args:
      engine_bin: Location of binary of the engine.
      opp_name: Name of opponent.

    Returns:
      Opened process handle.
    """
    cmd = [engine_bin]
    process_handle = subprocess.Popen(cmd, stdin=subprocess.PIPE,
                                      stdout=subprocess.PIPE)
    fd = process_handle.stdout.fileno()
    fl = fcntl.fcntl(fd, fcntl.F_GETFL)
    fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)
    print 'Starting instance', engine_bin
    process_handle.stdin.write('name ' + opp_name + '\n')
    process_handle.stdin.write('new\n')
    process_handle.stdin.write('variant suicide\n')
    return process_handle

  def _StartEngines(self):
    """Starts both engines."""
    self.engine1 = self._StartEngine(self.engine1_info.Binary(),
                                     self.engine2_info.Name())
    self.engine2 = self._StartEngine(self.engine2_info.Binary(),
                                     self.engine1_info.Name())

  def _ProcessCommand(self, line):
    """Processes the input line.

    Returns None if 'line' is not a valid command.
    """
    for x in valid_commands:
      if line.startswith(x):
        return line
    return None

  def _Result(self, line):
    if line.startswith('1-0'):
      return True, self.engine1_info.Name()
    elif line.startswith('0-1'):
      return True, self.engine2_info.Name()
    elif line.startswith('1/2-1/2'):
      return True, 'Draw'
    return False, None

  def _TimeCmd(self, engine, self_time, opp_time):
    """Issues time commands to the given engine."""
    engine.stdin.write('time %d\n' % self_time)
    engine.stdin.write('otim %d\n' % opp_time) 

  def _Communicate(self):
    """Communicates commands between two engines.

    If a line of output is available in the output stream of either of the
    engines, its read and written into the input stream of the other engine.
    If no output is available, it is simply skipped (it does a non blocking
    read).
    """
    got_out_1 = True
    got_out_2 = True
    try:
      out_1 = self.engine1.stdout.readline()
    except:
      got_out_1 = False
    try:
      out_2 = self.engine2.stdout.readline()
    except:
      got_out_2 = False

    if got_out_1:
      result_avail, winner = self._Result(out_1)
      if result_avail:
        self.is_game_end = True
        self.winner = winner
        return False, False

      command = self._ProcessCommand(out_1)
      if command:
        if command.startswith('move'):
          command = command.replace('move', 'usermove')
          self.engine1_remaining -= self._TimeNowCentis() - self.engine1_start
          self.engine2_start = self._TimeNowCentis()
          self._TimeCmd(self.engine2, self.engine2_remaining, self.engine1_remaining)
          self.total_moves += 1
        self.engine2.stdin.write(command)

    if got_out_2:
      result_avail, winner = self._Result(out_2)
      if result_avail:
        self.is_game_end = True
        self.winner = winner
        return False, False

      command = self._ProcessCommand(out_2)
      if command:
        if command.startswith('move'):
          command = command.replace('move', 'usermove')
          self.engine2_remaining -= self._TimeNowCentis() - self.engine2_start
          self.engine1_start = self._TimeNowCentis()
          self._TimeCmd(self.engine1, self.engine1_remaining, self.engine2_remaining)
          self.total_moves += 1
        self.engine1.stdin.write(command)

  def Play(self):
    """Begins the game and returns the result of the game."""
    self._StartEngines()

    self.engine1.stdin.write('go\n')

    self.engine1_start = self._TimeNowCentis()
    self.engine2_start = self._TimeNowCentis()
    self.engine1_remaining = self.seconds * 100
    self.engine2_remaining = self.seconds * 100

    total_moves = 0

    while True:
      self._Communicate()
      if self.is_game_end:
        break
      if self.total_moves > 300:
        self.is_game_end = True
        self.winner = 'Draw'
        break

    self.engine1.stdin.write('quit\n')
    self.engine2.stdin.write('quit\n')
    self.engine1.wait()
    self.engine2.wait()
    return self.winner

class Executor(Thread):
  def __init__(self, engine1, engine2, seconds, matches):
    Thread.__init__(self)
    self.results_file = open('results_log.txt', 'w')
    self.winner = []
    self.engine1 = engine1
    self.engine2 = engine2
    self.seconds = seconds
    self.matches = matches

  def run(self):
    for i in range(0, self.matches):
      if i % 2 == 0:
        game = Game(self.engine1, self.engine2, self.seconds)
      else:
        game = Game(self.engine2, self.engine1, self.seconds)
      winner = game.Play()
      print >>self.results_file, winner
      self.results_file.flush()
      self.winner.append(winner)

  def result(self):
    return self.winner

class ParallelExecutor(object):
  def __init__(self, engine1, engine2, seconds, matches, parallelism=1):
    self.engine1 = engine1
    self.engine2 = engine2
    self.seconds = seconds
    self.matches = matches
    self.parallelism = parallelism

  def Execute(self):
    matches_per_thread = self.matches / self.parallelism
    matches_last_thread = matches_per_thread + (
        self.matches - matches_per_thread * self.parallelism)

    executors = []
    for i in xrange(0, self.parallelism - 1):
      executor = Executor(self.engine1, self.engine2,
          self.seconds, matches_per_thread)
      executors.append(executor)
      executor.start()

    executor = Executor(self.engine1, self.engine2,
        self.seconds, matches_last_thread)
    executors.append(executor)
    executor.start()

    all_results = []
    for executor in executors:
      executor.join()
      all_results += executor.result()

    return all_results

def main():
  assert len(sys.argv) == 6
  engine1_name, engine1_bin = sys.argv[1].split(':')
  engine2_name, engine2_bin = sys.argv[2].split(':')
  seconds = int(sys.argv[3])
  matches = int(sys.argv[4])
  parallelism = int(sys.argv[5])

  engine1 = EngineInfo(engine1_name, engine1_bin)
  engine2 = EngineInfo(engine2_name, engine2_bin)

  results = None

  parallel_executor = ParallelExecutor(engine1, engine2, seconds,
      matches, parallelism)
  results = parallel_executor.Execute()
  print 'Final results:', results,

  e1 = 0
  e2 = 0
  draw = 0
  unknown = 0
  for result in results:
    if result == engine1.Name():
      e1 += 1
    elif result == engine2.Name():
      e2 += 1
    elif result == 'Draw':
      draw += 1
    else:
      unknown += 1
  print ' %s: %d ' % (engine1.Name(), e1),
  print ' %s: %d ' % (engine2.Name(), e2),
  print ' Draws: %d ' % draw,
  print ' Unknown: %d ' % unknown


if __name__ == '__main__':
  main()
