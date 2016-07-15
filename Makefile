kufs: kufs.o
	gcc main.c -o kufs kufs.o -lm

kufs.o: 
	gcc -c kufs.c -o kufs.o

clean:	
	$(RM) *.o kufs *~;
