Parallel Calculation of an Inverted Index Using the Map-Reduce Paradigm

Author

Birleanu Teodor MateiDate: 07.12.2024Email: teodor.matei.birleanu@gmail.com

Table of Contents

Introduction

Implementation Overview

Data Structures

Synchronization

File Distribution

How to Run

Acknowledgments

Introduction

This project demonstrates the parallel calculation of an inverted index using the Map-Reduce paradigm. The implementation follows a multi-threaded approach with proper synchronization mechanisms.

Implementation Overview

The implementation consists of two main operations:

Map Operation:

Assigns files to threads based on size.

Threads process files and store results in local maps.

Reduce Operation:

Threads process results from Map threads.

Each thread is assigned specific letters.

Final results are written to output files.

Data Structures

Map Threads

std::map<std::string, pair<int, int>>: Maps file names to IDs and sizes.

vector<string>: List of file names assigned to each thread.

Results stored in: std::map<std::string, set<int>>.

Reduce Threads

Receive a pointer to the Map threads’ results.

Assigned specific letters for processing using the assign_letters() function.

Results aggregated in a shared list: std::map<std::string, set<int>>.

Synchronization

Barrier:

After the Map operation to ensure all Mappers finish before starting the Reduce phase.

Mutex:

Used only in the Reduce operation when updating the shared aggregated list.

File Distribution

Files are sorted by size.

Distributed statically to threads with the smallest workload using the make_files_split() function.

How to Run

Clone the repository:

git clone <repository-url>

Compile the code:

make

Run the executable:

./inverted_index <input-files>

Acknowledgments

Thank you for reviewing this implementation. For further inquiries or contributions, please contact me at teodor.matei.birleanu@gmail.com.

Copyright © 2024 Birleanu Teodor Matei
