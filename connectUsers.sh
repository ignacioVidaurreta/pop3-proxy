for i in $(eval echo {0..$1})
do
	nc localhost 1110 &
done

