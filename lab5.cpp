//Iosif Nicolaou
//ID:UC10xxxxx

//libraries
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <set>
#include <cstdint>
#include <cmath>
#include <limits>
#include <chrono>
#include <iomanip>

using namespace std;

// Orismoi gia types cache kai replacement policy
// enum gia kainourio type cache: DM, SA, FA
enum CacheType { DM_TYPE, SA_TYPE, FA_TYPE };
enum ReplacementPolicyType { LRU_POLICY, SRRIP_POLICY };

// Struct pou anaferetai se ena block tis cache
struct CacheBlock {
    uint64_t address;    // dieuthynsi sto block
    unsigned int counter; // counter gia LRU/SRRIP
    bool valid;          // flag egkyris katastasis
    CacheBlock(): address(0), counter(0), valid(false) {}
};

class Cache {
    public:
        unsigned int totalQueries;  // posa access exei ginei
        unsigned int hits;          // posa hits
        unsigned int misses;        // posa misses
        unsigned int blocks;        // sinolikos arithmos blocks
        unsigned int ways;          // arithmos ways ana set
        unsigned int numSets;       // posa sets exei
        vector< vector<CacheBlock> > sets; // pinaka sets x blocks x ways

        // Constructor bazei counters sto 0
        Cache(unsigned int blocks, unsigned int ways)
            : totalQueries(0), hits(0), misses(0), blocks(blocks), ways(ways) {}
        virtual ~Cache() {}

        // virtual method gia access me address
        virtual void access(uint64_t address) = 0;

        // method gia print periexomenon cache sto file
        virtual void printCache(ofstream &fout) = 0;
    };

// Direct-Mapped Cache: 1 block ana set
class DirectMappedCache : public Cache {
    public:
        // constructor me blocks sifona me sets
        DirectMappedCache(unsigned int blocks) : Cache(blocks, 1) {
            numSets = blocks;
            sets.resize(numSets, vector<CacheBlock>(1));
        }

        // access method apo DM cache
        virtual void access(uint64_t address) {
            totalQueries++;  //  access pou exi gini
            unsigned int index = address % numSets;  // ypologismos index me modulo
            if (sets[index][0].valid && sets[index][0].address == address) {
                hits++;  // an egkuro kai isa me address -> hit
            } else {
                misses++;
                // antikatastasi simple: allagh address
                sets[index][0].address = address;
                sets[index][0].valid = true;
            }
        }

        // print periexomena cache sto fout
        virtual void printCache(ofstream &fout) {
            for (unsigned int i = 0; i < numSets; i++) {
                fout << "# " << i << " ";
                if (sets[i][0].valid)
                    fout << sets[i][0].address;
                else
                    fout << "0";
                fout << "\n";
            }
        }
    };

// Set-Associative Cache: ypostirizei LRU kai SRRIP
class SetAssociativeCache : public Cache {
    public:
        ReplacementPolicyType policy; // poia policy xrhsimopoieitai
        unsigned int maxRRPV;         // gia SRRIP: megisto RRPV = 2^M -1

        // constructor me blocks, ways, policy, kai bits M gia SRRIP
        SetAssociativeCache(unsigned int blocks, unsigned int ways, ReplacementPolicyType policy, unsigned int M = 0)
            : Cache(blocks, ways), policy(policy)
        {
            numSets = blocks / ways;
            if(numSets == 0) numSets = 1;
            sets.resize(numSets, vector<CacheBlock>(ways));

            if (policy == LRU_POLICY) {
                // arxikopoihsh LRU: counters =0, valid=false
                for (unsigned int i = 0; i < numSets; i++)
                    for (unsigned int j = 0; j < ways; j++) {
                        sets[i][j].counter = 0;
                        sets[i][j].valid = false;
                    }
            } else if (policy == SRRIP_POLICY) {
                // megisto RRPV
                maxRRPV = (1u << M) - 1;
                // counters = maxRRPV kai valid=false
                for (unsigned int i = 0; i < numSets; i++)
                    for (unsigned int j = 0; j < ways; j++) {
                        sets[i][j].counter = maxRRPV;
                        sets[i][j].valid = false;
                    }
            }
        }

    virtual void access(uint64_t address) {
        totalQueries++;
        unsigned int index = address % numSets;
        unsigned int accessedIndex = ways; // Αρχικοποίηση με τιμή εκτός εύρους (π.χ. ways)

        if (policy == LRU_POLICY) {
            bool hitFlag = false;

             // elegxos gia hit
            for (unsigned int i = 0; i < ways; i++) {
                if (sets[index][i].valid && sets[index][i].address == address) {
                    hits++;
                    sets[index][i].counter = 0;// reset counter me LRU
                    accessedIndex = i;
                    hitFlag = true;
                    break;
                }
            }

            if (!hitFlag) {
                misses++;
                bool placed = false;
                // an vrethei invalid block,
                for (unsigned int i = 0; i < ways; i++) {
                    if (!sets[index][i].valid) {
                        sets[index][i].address = address;
                        sets[index][i].valid = true;
                        sets[index][i].counter = 0;
                        accessedIndex = i;
                        placed = true;
                        break;
                    }
                }
                // vriski replacementcandidate me megalytero counter
                if (!placed) {
                    unsigned int replacementCandidate = 0;
                    unsigned int maxCounter = sets[index][0].counter;
                    for (unsigned int i = 1; i < ways; i++) {
                        if (sets[index][i].counter > maxCounter) {
                            maxCounter = sets[index][i].counter;
                            replacementCandidate = i;
                        }
                    }
                    sets[index][replacementCandidate].address = address;
                    sets[index][replacementCandidate].counter = 0;
                    accessedIndex = replacementCandidate;
                }
            }

            // ananeose counters gia ola ektos accessedIndex
            for (unsigned int i = 0; i < ways; i++) {
                if (i == accessedIndex) continue;  // Παράβλεψη του block που μόλις ανανεώθηκε
                if (sets[index][i].valid && sets[index][i].counter < numeric_limits<unsigned int>::max())
                    sets[index][i].counter++;
            }
        }
        else if (policy == SRRIP_POLICY) {
            bool hitFlag = false;
            // elegxos hit me SRRIP
            for (unsigned int i = 0; i < ways; i++) {
                if (sets[index][i].valid && sets[index][i].address == address) {
                    hits++;
                    sets[index][i].counter = 0;// reset RRPV
                    hitFlag = true;
                    break;
                }
            }
            if (!hitFlag) {
                misses++;
                for (unsigned int i = 0; i < ways; i++) {
                    if (!sets[index][i].valid) {
                        sets[index][i].address = address;
                        sets[index][i].valid = true;
                        sets[index][i].counter = maxRRPV - 1;
                        hitFlag = true;
                        break;
                    }
                }
                 // an den yparxei invalid, kane refill RRPV kai vres replacementcandidate
                while (!hitFlag) {
                    bool foundVictim = false;
                    unsigned int replacementCandidate = 0;
                    for (unsigned int i = 0; i < ways; i++) {
                        if (sets[index][i].counter == maxRRPV) {
                            replacementCandidate = i;
                            foundVictim = true;
                            break;
                        }
                    }
                    if (foundVictim) {
                        sets[index][replacementCandidate].address = address;
                        sets[index][replacementCandidate].counter = maxRRPV - 1;
                        sets[index][replacementCandidate].valid = true;
                        hitFlag = true;
                    } else {
                        for (unsigned int i = 0; i < ways; i++) {
                            if (sets[index][i].counter < maxRRPV)
                                sets[index][i].counter++;
                        }
                    }
                }
            }
        }
    }


    virtual void printCache(ofstream &fout) {
        for (unsigned int i = 0; i < numSets; i++) {
            fout << "# " << i << " ";
            for (unsigned int j = 0; j < ways; j++) {
                if (sets[i][j].valid)
                    fout << sets[i][j].address << " ";
                else
                    fout << "0 ";
            }
            fout << "\n";
        }
    }
};

// Fully Associative Cache: είναι ουσιαστικά set-associative με 1 set και number_of_ways = total blocks.
class FullyAssociativeCache : public SetAssociativeCache {
public:

// constructor pou kalei SetAssociativeCache me 1 set, ways = blocks
    FullyAssociativeCache(unsigned int blocks, ReplacementPolicyType policy, unsigned int M = 0)
        : SetAssociativeCache(blocks, blocks, policy, M)
    {
        numSets = 1;
        sets.resize(1, vector<CacheBlock>(blocks));
        if (policy == LRU_POLICY) {
            for (unsigned int i = 0; i < blocks; i++) {
                sets[0][i].counter = 0;
                sets[0][i].valid = false;
            }
        } else if (policy == SRRIP_POLICY) {
            maxRRPV = (1u << M) - 1;
            for (unsigned int i = 0; i < blocks; i++) {
                sets[0][i].counter = maxRRPV;
                sets[0][i].valid = false;
            }
        }
    }
};

//
// ------------------------- Main ---------------------------
//

int main() {
    // zita to onoma arxeiou me dieuthinseis
    string inputFileName;
    cout << "Enter input file name with memory addresses: ";
    cin >> inputFileName;

    ifstream fin(inputFileName);
    if (!fin.is_open()) {
        cerr << "Cannot open input file.\n";
        return 1;
    }

   // diabazoume oles tis dieuthinseis apo to arxeio
    vector<uint64_t> addresses;
    addresses.clear();

    string line;


    //zita mege8os cache se blocks
    unsigned int memoryBlocks;
    cout << "Enter total number of blocks in cache: ";
    cin >> memoryBlocks;

    // zita type cache
    string cacheTypeStr;
    cout << "Enter cache type (DM, SA, FA): ";
    cin >> cacheTypeStr;

    // dianomi dieuthinseon se vector
    while (getline(fin, line)) {
        if (line.find_first_not_of(" \t\r\n") == string::npos)
            continue;               // skip empty or whitespace-only lines
        stringstream ss(line);
        uint64_t addr;
        ss >> addr;
        addresses.push_back(addr);
    }

    fin.close();

     // metra monadikes dieuthinsis
    set<uint64_t> uniqueAddrs(addresses.begin(), addresses.end());
    unsigned int totalUnique = uniqueAddrs.size();

    CacheType cacheType;
    if (cacheTypeStr == "DM" || cacheTypeStr == "dm" || cacheTypeStr == "Dm") {
        cacheType = DM_TYPE;
    } else if (cacheTypeStr == "SA" || cacheTypeStr == "sa" || cacheTypeStr == "Sa") {
        cacheType = SA_TYPE;
    } else if (cacheTypeStr == "FA" || cacheTypeStr == "fa" || cacheTypeStr == "Fa") {
        cacheType = FA_TYPE;
    } else {
        cerr << "Invalid cache type.\n";
        return 1;
    }

    unsigned int ways = 1;
    ReplacementPolicyType repPolicy = LRU_POLICY; // προεπιλογή
    unsigned int M = 0; // μόνο για την πολιτική SRRIP

    // an SA, zita ways kai policy
    if (cacheType == SA_TYPE) {
        cout << "Enter number of ways: ";
        cin >> ways;
        cout << "Enter replacement policy (LRU or SRRIP): ";
        string repPolicyStr;
        cin >> repPolicyStr;
        if (repPolicyStr == "LRU" || repPolicyStr == "lru" || repPolicyStr == "Lru") {
            repPolicy = LRU_POLICY;
        } else if (repPolicyStr == "SRRIP" || repPolicyStr == "srrip" || repPolicyStr == "Srrip") {
            repPolicy = SRRIP_POLICY;
            cout << "Enter number of bits for RRPV (M): ";
            cin >> M;
            if(M == 0) {
                cerr << "M must be > 0 for SRRIP.\n";
                return 1;
            }
        } else {
            cerr << "Invalid replacement policy.\n";
            return 1;
        }
    // an FA, ways = blocks kai zita policy
    } else if (cacheType == FA_TYPE) {
        // Για FA, οι ways είναι ίσες με το πλήθος των blocks
        ways = memoryBlocks;
        cout << "Enter replacement policy (LRU or SRRIP): ";
        string repPolicyStr;
        cin >> repPolicyStr;
        if (repPolicyStr == "LRU" || repPolicyStr == "lru" || repPolicyStr == "Lru") {
            repPolicy = LRU_POLICY;
        } else if (repPolicyStr == "SRRIP" || repPolicyStr == "srrip" || repPolicyStr == "Srrip") {
            repPolicy = SRRIP_POLICY;
            cout << "Enter number of bits for RRPV (M): ";
            cin >> M;
            if(M == 0) {
                cerr << "M must be > 0 for SRRIP.\n";
                return 1;
            }
        } else {
            cerr << "Invalid replacement policy.\n";
            return 1;
        }
    }

    // dimiourgia arxiou eksodou
    string outputFileName;

    // dimiourgia cache antikeimenou me epiloges xrhsth
    Cache* cache = nullptr;
    if (cacheType == DM_TYPE)
    {
        cache = new DirectMappedCache(memoryBlocks);
        outputFileName= "Iosif_Nicolaou_UC10xxxxx_A.txt";
    }

    else if (cacheType == SA_TYPE)
    {
        cache = new SetAssociativeCache(memoryBlocks, ways, repPolicy, M);

        if(repPolicy == LRU_POLICY)
            outputFileName= "Iosif_Nicolaou_UC10xxxxx_C.txt";
        else
            outputFileName= "Iosif_Nicolaou_UC10xxxxx_D.txt";

    }

    else if (cacheType == FA_TYPE)
    {
        cache = new FullyAssociativeCache(memoryBlocks, repPolicy, M);

        if(repPolicy == LRU_POLICY)
            outputFileName= "Iosif_Nicolaou_UC10xxxxx_.txt";
        else
            outputFileName= "Iosif_Nicolaou_UC10xxxxx_B.txt";

    }

    // ekkinisi xronou simulatoin
        auto start = chrono::high_resolution_clock::now();

     // trexoume ola ta addresses
    for (size_t i = 0; i < addresses.size(); i++) {
        cache->access(addresses[i]);
    }


    // ypologismos rates
    double hitRate = (cache->hits * 100.0) / cache->totalQueries;
    double missRate = 100.0 - hitRate;


    ofstream fout(outputFileName);
    if (!fout.is_open()) {
        cerr << "Cannot open output file.\n";
        return 1;
    }

    //ipologismos xronou simulation
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double, std::nano> elapsed = end - start;  // nanoseconds ως double
    double elapsed_ns = elapsed.count();

    // Εξαγωγή στατιστικών και των περιεχομένων της cache
    fout << "Cache_accesses = " << cache->totalQueries << "\n";
    fout << "Total_number_of_unique_addresses = " << totalUnique << "\n";
    fout << "Total_number_of_misses = " << cache->misses << "\n";
    fout << "Total_number_of_hits = " << cache->hits << "\n";
    fout << "Miss_rate = " <<fixed << setprecision(3)<< missRate << "\n";
    fout << "Hit_rate = " << fixed << setprecision(3)<<hitRate << "\n";
    fout << "User time = "  << fixed << setprecision(3)<< elapsed_ns << "ns\n\n";

    fout << "--------Cache Contents---------\n\n";
    cache->printCache(fout);

    fout.close();

    cout << "Simulation complete. Output written to " << outputFileName << "\n";

    delete cache;
    return 0;
}
