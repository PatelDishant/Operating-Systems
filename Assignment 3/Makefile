all: helper ext2_ls ext2_ln ext2_mkdir ext2_rm ext2_cp

helper: helper.o

ext2_ls:  ext2_ls.o
	gcc -Wall -g -o ext2_ls $^

ext2_ln: ext2_ln.o
	gcc -Wall -g -o ext2_ln $^

ext2_cp: ext2_cp.o
	gcc -Wall -g -o ext2_cp $^

ext2_rm: ext2_rm.o
	gcc -Wall -g -o ext2_rm $^

ext2_mkdir: ext2_mkdir.o
	gcc -Wall -g -o ext2_mkdir $^

test_helper :  helper.o test_helper.o
	gcc -Wall -g -o test_helper $^

%.o : %.c helper.h ext2.h
	gcc -Wall -g -c $<

clean : 
	rm -f *.o *~
