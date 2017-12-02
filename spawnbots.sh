if [ $# < 2 ]
then
	echo "Specify: <channel> <num bots>"
else
	for (( i = 0; i < $2; i++ ))
	do
		./bot 162.246.156.17 12399 $1 secret &
	done	
fi
	