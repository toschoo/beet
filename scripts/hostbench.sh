rm -r rsc/idx200
rm -r rsc/idx201
bin/beet create rsc/idx200 -type 1 -leaf 30 -internal 42 -key 8 -data 0 \
                           -compare "beetSmokeUInt64Compare"
bin/beet create rsc/idx201 -type 3 -leaf 255 -internal 340 -key 8 \
                           -subpath "rsc/idx200" \
                           -compare "beetSmokeUInt64Compare"
bin/writebench host rsc/idx201 -count 1000 -random false
