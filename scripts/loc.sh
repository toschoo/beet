c=0
for f in $(ls src/beet)
do
	stp=$(wc -l src/beet/$f | awk '{print $1}')
	c=$(($c+$stp))
done
printf "beet: % 6d\n" $c
