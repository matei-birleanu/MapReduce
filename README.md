# Parallel Calculation of an Inverted Index Using the Map-Reduce Paradigm

## Author
**Birleanu Teodor Matei**  
**Date:** 07.12.2024  
**Email:** teodor.matei.birleanu@gmail.com  

---

## Table of Contents
- [Introduction](#introduction)
- [Implementation Overview](#implementation-overview)
- [Data Structures](#data-structures)
- [Synchronization](#synchronization)
- [File Distribution](#file-distribution)
- [How to Run](#how-to-run)
- [Acknowledgments](#acknowledgments)

---

## Introduction
This project demonstrates the parallel calculation of an inverted index using the Map-Reduce paradigm. The implementation follows a multi-threaded approach with proper synchronization mechanisms.

---

## Implementation Overview
The implementation consists of two main operations:

1. **Map Operation:**
   - Assigns files to threads based on size.
   - Threads process files and store results in local maps.

2. **Reduce Operation:**
   - Threads process results from Map threads.
   - Each thread is assigned specific letters.
   - Final results are written to output files.

---

## Data Structures
### Map Threads
- `std::map<std::string, pair<int, int>>`: Maps file names to IDs and sizes.
- `vector<string>`: List of file names assigned to each thread.
- Results stored in: `std::map<std::string, set<int>>`.

### Reduce Threads
- Receive a pointer to the Map threads’ results.
- Assigned specific letters for processing using the `assign_letters()` function.
- Results aggregated in a shared list: `std::map<std::string, set<int>>`.

---

## Synchronization
- **Barrier:**
  - After the Map operation to ensure all Mappers finish before starting the Reduce phase.

- **Mutex:**
  - Used only in the Reduce operation when updating the shared aggregated list.

---

## File Distribution
- Files are sorted by size.
- Distributed statically to threads with the smallest workload using the `make_files_split()` function.

---

## How to Run
1. Clone the repository:
   ```bash
   git clone <repository-url>
