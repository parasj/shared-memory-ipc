Shared Memory IPC System
------------------------

To build:
1. Under the snappy-c directory, run `make`
2. In the root directory, run `mkdir bin`
2. Run `make clean`
3. Run `make`

To run the server, use: `./bin/tinyd`

To adjust the number and size of shared memory segments per client, adjust the parameters for: `./bin/tinyd -n 123 -s 123` where n represents number and s represents size of shared segments

Once the server is running, you can test the application using: `./bin/tiny data/alice.txt data/pride.txt` where the arguments are a list of files.
To test the client's async API, specify the -a flag (`./bin/tiny -a data/alice.txt data/pride.txt`)