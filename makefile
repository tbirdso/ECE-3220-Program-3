bin : tfs_1.c tfs_2.c tfs_driver1.c
	gcc -o tfs.out tfs_1.c tfs_2.c tfs_driver1.c

run : tfs.out
	./tfs.out

run2 : tfs_1.c tfs_2.c tfs_driver2.c
	gcc -o tfs.out tfs_1.c tfs_2.c tfs_driver2.c
	./tfs.out

debug : tfs_1.c tfs_2.c tfs_driver1.c
	gcc -g -o tfs.out tfs_1.c tfs_2.c tfs_driver1.c
	gdb ./tfs.out

debug2 : tfs_1.c tfs_2.c tfs_driver2.c
	gcc -g -o tfs.out tfs_1.c tfs_2.c tfs_driver2.c
	gdb ./tfs.out

clean : tfs.out
	rm tfs.out
