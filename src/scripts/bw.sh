{
#
# Dummy flows
#
echo "insert into Flows \
values ('6', \"192.168.1.1\", '8080', \"10.0.0.1\", '1025', '1', '100')\n";
echo "insert into Flows \
values ('6', \"192.168.1.1\", '8080', \"10.0.0.2\", '1025', '1', '100')\n";
sleep 3
#
# Change the allowance of a device
#
echo "insert into Allowances \
values (\"10.0.0.1\", '1000') on duplicate key update\n";
sleep 1
echo "insert into Flows \
values ('6', \"192.168.1.1\", '8080', \"10.0.0.1\", '1025', '1', '100')\n";
sleep 60
echo "select * from BWUsage\n";

} | ./cacheclient -l packets $*

exit 0

