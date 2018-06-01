while true
do
	#valgrind
	test/smoke/contreesmoke -threads 100 2>err.txt
	if [ $? -ne 0 ]
	then
		break
	fi
done
