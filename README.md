<h1>POSIX-compliant file indexer</h1>

<h3>1. Purpose</h3>
This project was developed as part of my education in the field of C-programs for unix-like systems.
Its main goal is to traverse the file tree in a specified directory and scan for files of strictly defined types.
The program does create a dictionary-like structure, however it is not efficient or optimised.
The project does not focus on any advanced data structures, in fact, this aspect is completely omitted and neglected.

<h3>2. Prerequisites</h3>
<ul>
    <li>linux or unix-like environment (<i>pthread</i>)</li>
    <li>bash-like command tool</li>
    <li>gcc</li>
    <li><i>it works well in WSL</i></li>
</ul>

<h3>3. Installation</h3>

```
cd proj_folder
make
```

<b>WARNING!</b><br>

Some linkers will throw errors because of the `-lpthread` flag. If that is happening in your case as well, try this command: 
```
gcc -o mole -std=gnu99 -Wall -lm mole.c priority_queue.c globals.c index.c stack.c -lpthread 
```
placing the `-lpthread` flag at the end should do the trick. <br>
If there are still some errors try substituting `-lpthread` with `-pthread`.

<h4>4. Usage</h4>
Start the indexing program with:

```
mole -d <directory> [-f <index-file>] [-t <time interval for periodic indexing>]
```

If `-d` is not specified, the program will use the path from environmental variable `$MOLE_DIR`.
If neither the variable and `-d` are provided, the program ends with an error. 

If `-f` is not specified, the program will use the file from environmental variable `$MOLE_INDEX_PATH`.
In case neither the variable and `-f` are specified, the index-file location will be set to `~/mole-index`.

If `-t` is not specified, then the periodic indexing is off. 

During the word of the mole use these commands:
<br> ```index``` - initiate indexing procedure
<br>```exit``` - finishes ongoing word, saves progress and quits
<br>```exit!``` - quits abruptly
<br> ```largerthan <number>``` - lists files larger than `<number`
<br> ```count``` - lists the amount of files of recognisable types that are in the index
<br> ```namepart <string>``` - lists the files which name consists the `<string>`
<br> ```owner <uid>``` - lists the files that are owned by user `<uid>`

