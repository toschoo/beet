if [ $# -lt 1 ]
then
	echo "I need a base path!"
	exit 1 
fi

base=$1
emb=$1/idx200
host=$1/idx201

echo "Create index $host and $emb?"
read

if [ "$REPLY" != "y" ]
then
	echo "ok, exiting"
	exit 1
fi

stat "$emb" >/dev/null 2>&1
if [ $? -eq 0 ]
then
	echo "removing $emb"
	rm -r $emb
fi

stat "$host" >/dev/null 2>&1
if [ $? -eq 0 ]
then
	echo "removing $host"
	rm -r $host
fi

bin/beet create $emb -type 1 -leaf 30 -internal 42 -key 8 \
	             -standalone false \
                     -compare "beetSmokeUInt64Compare"

if [ $? -ne 0 ]
then
	echo "cannot create $emb"
	exit 1
fi

bin/beet create $host -type 3 -leaf 340 -internal 340 -key 8 \
                      -subpath "$emb" \
                      -compare "beetSmokeUInt64Compare"
if [ $? -ne 0 ]
then
	echo "cannot create $host"
	exit 1
fi

bin/writebench host $host -count 10000 -random false

if [ $? -ne 0 ]
then
	echo "cannot write to $host"
	exit 1
fi

