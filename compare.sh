for f in $*; do
echo $f; tar -zxOf ../adts.tgz $f | diff - $f
done
