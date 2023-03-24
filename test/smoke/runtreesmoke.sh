it=12

if [ "$1" != "" ]
then
	it=$1
fi

echo "" > out.log

for i in $(seq 1 $it)
do
	echo "running iteration $i of $it"
	test/smoke/treesmoke >> out.log 2>&1
	if [ $? -ne 0 ]
	then
		echo "error after $i iterations"
		exit
	fi
	sleep 1
done
