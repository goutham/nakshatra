if [ -z $1 ]; then
  VARIANT='suicide'
else
  VARIANT=$1
fi

echo $VARIANT

xboard \
  -fcp ./nakshatra \
  -size small \
  -variant $VARIANT \
  -debug \
  -timeControl 3:0 \
  -mps 0
