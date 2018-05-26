make smoke
if [ $? -ne 0 ]
then
	echo "FAILED: cannot make tests"
	exit 1
fi

echo "RUNNING TEST BATTERY 'SMOKE'" > log/test.log

test/smoke/pagesmoke >> log/test.log 2>&1
if [ $? -ne 0 ]
then
	echo "FAILED: pagesmoke failed"
	exit 1
fi

test/smoke/ridersmoke >> log/test.log 2>&1
if [ $? -ne 0 ]
then
	echo "FAILED: ridersmoke failed"
	exit 1
fi

echo "PASSED"
