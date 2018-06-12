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

test/smoke/nodesmoke >> log/test.log 2>&1
if [ $? -ne 0 ]
then
	echo "FAILED: nodesmoke failed"
	exit 1
fi

test/smoke/treesmoke >> log/test.log 2>&1
if [ $? -ne 0 ]
then
	echo "FAILED: treesmoke failed"
	exit 1
fi

test/smoke/embeddedsmoke >> log/test.log 2>&1
if [ $? -ne 0 ]
then
	echo "FAILED: embeddedsmoke failed"
	exit 1
fi

test/smoke/contreesmoke -threads 1 >> log/test.log 2>&1
if [ $? -ne 0 ]
then
	echo "FAILED: contreesmoke failed with 1 thread"
	exit 1
fi

test/smoke/contreesmoke -threads 2 >> log/test.log 2>&1
if [ $? -ne 0 ]
then
	echo "FAILED: contreesmoke failed with 2 thread"
	exit 1
fi

test/smoke/contreesmoke -threads 10 >> log/test.log 2>&1
if [ $? -ne 0 ]
then
	echo "FAILED: contreesmoke failed with 10 threads"
	exit 1
fi

test/smoke/contreesmoke -threads 100 >> log/test.log 2>&1
if [ $? -ne 0 ]
then
	echo "FAILED: contreesmoke failed with 100 threads"
	exit 1
fi

test/smoke/indexsmoke >> log/test.log 2>&1
if [ $? -ne 0 ]
then
	echo "FAILED: indexsmoke failed"
	exit 1
fi

test/smoke/hostsmoke >> log/test.log 2>&1
if [ $? -ne 0 ]
then
	echo "FAILED: hostsmoke failed"
	exit 1
fi

test/smoke/rscsmoke >> log/test.log 2>&1
if [ $? -ne 0 ]
then
	echo "FAILED: rscsmoke failed"
	exit 1
fi

test/smoke/rangesmoke >> log/test.log 2>&1
if [ $? -ne 0 ]
then
	echo "FAILED: rangesmoke failed"
	exit 1
fi

test/smoke/range2smoke >> log/test.log 2>&1
if [ $? -ne 0 ]
then
	echo "FAILED: range2smoke failed"
	exit 1
fi

echo "PASSED"
