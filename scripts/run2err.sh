while true
do
	#valgrind
	test/smoke/contreesmoke -threads 1000 2>err.txt
	if [ $? -ne 0 ]
	then
		break
	fi
done
