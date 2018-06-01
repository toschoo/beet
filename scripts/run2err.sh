while true
do
	# valgrind --tool=helgrind
	test/smoke/contreesmoke -threads 10 2>err.txt
	if [ $? -ne 0 ]
	then
		break
	fi
done
