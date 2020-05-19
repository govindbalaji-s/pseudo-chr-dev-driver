sshpass -p "r123" scp -r ../pseudo-chr-dev-driver/ root@192.168.122.158: ;
sshpass -p "r123" ssh root@192.168.122.158 'cd pseudo-chr-dev-driver; make; bash runtest.sh';
