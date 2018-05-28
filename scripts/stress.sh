make stress
if [ $? -ne 0 ]
then
	echo "FAILED: cannot make tests"
	exit 1
fi

echo "RUNNING STRESS TESTS"

test/stress/riderstress -threads 1 -max 1000
if [ $? -ne 0 ]
then
	exit 1
fi

test/stress/riderstress -threads 2 -max 1000
if [ $? -ne 0 ]
then
	exit 1
fi

test/stress/riderstress -threads 10 -max 100
if [ $? -ne 0 ]
then
	exit 1
fi

test/stress/riderstress -threads 100 -max 10
if [ $? -ne 0 ]
then
	exit 1
fi

test/stress/riderstress -threads 10 -max 100000
if [ $? -ne 0 ]
then
	exit 1
fi

test/stress/riderstress -threads 100 -max 100000
if [ $? -ne 0 ]
then
	exit 1
fi

test/stress/riderstress -threads 1000 -max 100000
if [ $? -ne 0 ]
then
	exit 1
fi

test/stress/riderstress -threads 1000 -max 100
if [ $? -ne 0 ]
then
	exit 1
fi

echo "PASSED"
