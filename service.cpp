#include "Utils/Data.h"
#include "Circuits/RingSettlementCircuit.h"
#include "Circuits/DepositCircuit.h"
#include "Circuits/OnchainWithdrawalCircuit.h"
#include "Circuits/OffchainWithdrawalCircuit.h"
#include "Circuits/OrderCancellationCircuit.h"

#include "ThirdParty/json.hpp"
#include "ethsnarks.hpp"
#include "stubs.hpp"
#include <fstream>

#ifdef MULTICORE
#include <omp.h>
#endif

#include "service.h"

using json = nlohmann::json;

enum class Mode
{
    CreateKeys = 0,
    Validate,
    Prove
};

timespec diff(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    }
    else
    {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

bool fileExists(const char *fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

bool generateKeyPair(ethsnarks::ProtoboardT& pb, std::string& baseFilename)
{
    std::string provingKeyFilename = baseFilename + "_pk.raw";
    std::string verificationKeyFilename = baseFilename + "_vk.json";
    if (fileExists(provingKeyFilename.c_str()) && fileExists(verificationKeyFilename.c_str()))
    {
        return true;
    }
    int result = stub_genkeys_from_pb(pb, provingKeyFilename.c_str(), verificationKeyFilename.c_str());
    return (result == 0);
}

std::string generateProof(ethsnarks::ProtoboardT& pb, const char *provingKeyFilename)
{
    std::string jProof = stub_prove_from_pb(pb, provingKeyFilename);
    return jProof;
}

bool trade(Mode mode, bool onchainDataAvailability, unsigned int numRings,
           const json& input, ethsnarks::ProtoboardT& outPb)
{
    // Build the circuit
    Loopring::RingSettlementCircuit circuit(outPb, "circuit");
    circuit.generate_r1cs_constraints(onchainDataAvailability, numRings);
    circuit.printInfo();

    if (mode == Mode::Validate || mode == Mode::Prove)
    {
        json jRingSettlements = input["ringSettlements"];
        if (jRingSettlements.size() != numRings)
        {
            std::cerr << "Invalid number of rings in input file: " << jRingSettlements.size() << std::endl;
            return false;
        }

        Loopring::RingSettlementBlock block = input.get<Loopring::RingSettlementBlock>();

        // Generate witness values for the given input values
        if (!circuit.generateWitness(block))
        {
            std::cerr << "Could not generate witness!" << std::endl;
            return false;
        }
    }
    return true;
}

bool deposit(Mode mode, unsigned int numDeposits, const json& input, ethsnarks::ProtoboardT& outPb)
{
    // Build the circuit
    Loopring::DepositCircuit circuit(outPb, "circuit");
    circuit.generate_r1cs_constraints(numDeposits);
    circuit.printInfo();

    if (mode == Mode::Validate || mode == Mode::Prove)
    {
        json jDeposits = input["deposits"];
        if (jDeposits.size() != numDeposits)
        {
            std::cerr << "Invalid number of deposits in input file: " << jDeposits.size() << std::endl;
            return false;
        }

        Loopring::DepositBlock block = input.get<Loopring::DepositBlock>();

        // Generate witness values for the given input values
        if (!circuit.generateWitness(block))
        {
            std::cerr << "Could not generate witness!" << std::endl;
            return false;
        }
    }
    return true;
}

bool onchainWithdraw(Mode mode, bool onchainDataAvailability, unsigned int numWithdrawals, const json& input, ethsnarks::ProtoboardT& outPb)
{
    // Build the circuit
    Loopring::OnchainWithdrawalCircuit circuit(outPb, "circuit");
    circuit.generate_r1cs_constraints(onchainDataAvailability, numWithdrawals);
    circuit.printInfo();

    if (mode == Mode::Validate || mode == Mode::Prove)
    {
        json jWithdrawals = input["withdrawals"];
        if (jWithdrawals.size() != numWithdrawals)
        {
            std::cerr << "Invalid number of withdrawals in input file: " << jWithdrawals.size() << std::endl;
            return false;
        }

        Loopring::OnchainWithdrawalBlock block = input.get<Loopring::OnchainWithdrawalBlock>();

        // Generate witness values for the given input values
        if (!circuit.generateWitness(block))
        {
            std::cerr << "Could not generate witness!" << std::endl;
            return false;
        }
    }
    return true;
}

bool offchainWithdraw(Mode mode, bool onchainDataAvailability, unsigned int numWithdrawals, const json& input, ethsnarks::ProtoboardT& outPb)
{
    // Build the circuit
    Loopring::OffchainWithdrawalCircuit circuit(outPb, "circuit");
    circuit.generate_r1cs_constraints(onchainDataAvailability, numWithdrawals);
    circuit.printInfo();

    if (mode == Mode::Validate || mode == Mode::Prove)
    {
        json jWithdrawals = input["withdrawals"];
        if (jWithdrawals.size() != numWithdrawals)
        {
            std::cerr << "Invalid number of withdrawals in input file: " << jWithdrawals.size() << std::endl;
            return false;
        }

        Loopring::OffchainWithdrawalBlock block = input.get<Loopring::OffchainWithdrawalBlock>();

        // Generate witness values for the given input values
        if (!circuit.generateWitness(block))
        {
            std::cerr << "Could not generate witness!" << std::endl;
            return false;
        }
    }
    return true;
}

bool cancel(Mode mode, bool onchainDataAvailability, unsigned int numCancels, const json& input, ethsnarks::ProtoboardT& outPb)
{
    // Build the circuit
    Loopring::OrderCancellationCircuit circuit(outPb, "circuit");
    circuit.generate_r1cs_constraints(onchainDataAvailability, numCancels);
    circuit.printInfo();

    if (mode == Mode::Validate || mode == Mode::Prove)
    {
        json jCancels = input["cancels"];
        if (jCancels.size() != numCancels)
        {
            std::cerr << "Invalid number of cancels in input file: " << jCancels.size() << std::endl;
            return false;
        }

        Loopring::OrderCancellationBlock block = input.get<Loopring::OrderCancellationBlock>();

        // Generate witness values for the given input values
        if (!circuit.generateWitness(block))
        {
            std::cerr << "Could not generate witness!" << std::endl;
            return false;
        }
    }
    return true;
}

std::string generateBaseFileName(const json& input) {
    // Read meta data
    int blockType = input["blockType"].get<int>();
    unsigned int blockSize = input["blockSize"].get<int>();
    bool onchainDataAvailability = input["onchainDataAvailability"].get<bool>();
    std::string strOnchainDataAvailability = onchainDataAvailability ? "_DA_" : "_";
    std::string postFix = strOnchainDataAvailability + std::to_string(blockSize);

    std::string baseFilename = "keys/";
    switch(blockType) {
    case 0:
        {
            baseFilename += "trade" + postFix;
            break;
        }
    case 1:
        {
            baseFilename += "deposit" + postFix;
            break;
        }
    case 2:
        {
            baseFilename += "withdraw_onchain" + postFix;
            break;
        }
    case 3:
        {
            baseFilename += "withdraw_offchain" + postFix;
            break;
        }
    case 4:
        {
            baseFilename += "cancel" + postFix;
            break;
        }
    default:
        {
            std::cerr << "Unknown block type: " << blockType << std::endl;
            return "";
        }
    }
    return baseFilename;
}

int runCircuit(Mode mode, const json& input, ethsnarks::ProtoboardT& pb) {
    // Read meta data
    int blockType = input["blockType"].get<int>();
    unsigned int blockSize = input["blockSize"].get<int>();
    bool onchainDataAvailability = input["onchainDataAvailability"].get<bool>();

    switch(blockType)
    {
        case 0:
        {
            if (!trade(mode, onchainDataAvailability, blockSize, input, pb))
            {
                return 1;
            }
            break;
        }
        case 1:
        {
            if (!deposit(mode, blockSize, input, pb))
            {
                return 1;
            }
            break;
        }
        case 2:
        {
            if (!onchainWithdraw(mode, onchainDataAvailability, blockSize, input, pb))
            {
                return 1;
            }
            break;
        }
        case 3:
        {
            if (!offchainWithdraw(mode, onchainDataAvailability, blockSize, input, pb))
            {
                return 1;
            }
            break;
        }
        case 4:
        {
            if (!cancel(mode, onchainDataAvailability, blockSize, input, pb))
            {
                return 1;
            }
            break;
        }
        default:
        {
            std::cerr << "Unknown block type: " << blockType << std::endl;
            return 1;
        }
    }
    return 0;
}

bool validateBlock(char* inputJson) {
    ethsnarks::ppT::init_public_params();

    json input = json::parse(inputJson);
    ethsnarks::ProtoboardT pb;
    runCircuit(Mode::Validate, input, pb);

    // Check if the inputs are valid for the circuit
    if (!pb.is_satisfied())
    {
        return false;
    }
    return true;
}

bool createKeyPair(char* inputJson) {
    ethsnarks::ppT::init_public_params();

    json input = json::parse(inputJson);
    std::string baseFilename = generateBaseFileName(input);
    ethsnarks::ProtoboardT pb;
    runCircuit(Mode::CreateKeys, input, pb);

    if (!generateKeyPair(pb, baseFilename)) {
        return false;
    }
    return true;
}

ProofResult generateProof(char* inputJson) {
    ProofResult res;
    ethsnarks::ppT::init_public_params();

#ifdef MULTICORE
    const int max_threads = omp_get_max_threads();
#endif

    json input = json::parse(inputJson);
    std::string baseFilename = generateBaseFileName(input);
    ethsnarks::ProtoboardT pb;
    runCircuit(Mode::Prove, input, pb);

    // Check if the inputs are valid for the circuit
    if (!pb.is_satisfied()) {
        res.success = false;
        res.errorMessage = "Block is not valid";
        return res;
    }

    if (!generateKeyPair(pb, baseFilename)) {
        res.success = false;
        res.errorMessage = "Failed to generate keys:" + baseFilename;
        return res;
    }

    timespec time1, time2;
    clock_gettime(CLOCK_MONOTONIC, &time1);
    std::string proofJsonStr = generateProof(pb, (baseFilename + "_pk.raw").c_str());
    clock_gettime(CLOCK_MONOTONIC, &time2);
    timespec duration = diff(time1,time2);

    if (!proofJsonStr.empty()) {
        res.success = false;
        res.errorMessage = "Failed to generate Proof:" + baseFilename + "_pk.raw";
        return res;
    }

    res.success = true;
    res.costSeconds = duration.tv_sec;
    res.proofJsonStr = proofJsonStr;
    return res;
}

