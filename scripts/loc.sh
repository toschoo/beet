c=0
for f in $(ls -l src/beet | awk '{print $9}')
do
	stp=$(wc -l src/beet/$f | awk '{print $1}')
	printf "% 9s: % 6d\n" $f $stp
	c=$(($c+$stp))
done
printf -- "-----------------\n"
printf "% 9s: % 6d\n" "beet" $c
