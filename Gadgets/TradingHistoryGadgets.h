#ifndef _TRADINGHISTORYCIRCUIT_H_
#define _TRADINGHISTORYCIRCUIT_H_

#include "../Utils/Constants.h"
#include "../Utils/Data.h"

#include "ethsnarks.hpp"
#include "utils.hpp"
#include "gadgets/mimc.hpp"
#include "gadgets/merkle_tree.hpp"

using namespace ethsnarks;

namespace Loopring
{

struct TradeHistoryState
{
    const VariableT filled;
    const VariableT cancelled;
    const VariableT orderID;
};

class UpdateTradeHistoryGadget : public GadgetT
{
public:
    typedef merkle_path_authenticator<MiMC_hash_gadget> MerklePathCheckT;
    typedef markle_path_compute<MiMC_hash_gadget> MerklePathT;

    const VariableT merkleRootBefore;

    libsnark::dual_variable_gadget<FieldT> fill;

    MiMC_hash_gadget leafBefore;
    MiMC_hash_gadget leafAfter;

    const VariableArrayT proof;
    MerklePathCheckT proofVerifierBefore;
    MerklePathT rootCalculatorAfter;

    UpdateTradeHistoryGadget(
        ProtoboardT& pb,
        const VariableT& _merkleRoot,
        const VariableArrayT& address,
        const TradeHistoryState& before,
        const TradeHistoryState& after,
        const std::string& prefix
    ) :
        GadgetT(pb, prefix),

        merkleRootBefore(_merkleRoot),

        fill(pb, NUM_BITS_AMOUNT, FMT(prefix, ".fill")),

        leafBefore(pb, libsnark::ONE, {before.filled, before.cancelled, before.orderID}, FMT(prefix, ".leafBefore")),
        leafAfter(pb, libsnark::ONE, {after.filled, after.cancelled, after.orderID}, FMT(prefix, ".leafAfter")),

        proof(make_var_array(pb, TREE_DEPTH_TRADING_HISTORY, FMT(prefix, ".proof"))),
        proofVerifierBefore(pb, TREE_DEPTH_TRADING_HISTORY, address, merkle_tree_IVs(pb), leafBefore.result(), merkleRootBefore, proof, FMT(prefix, ".pathBefore")),
        rootCalculatorAfter(pb, TREE_DEPTH_TRADING_HISTORY, address, merkle_tree_IVs(pb), leafAfter.result(), proof, FMT(prefix, ".pathAfter"))
    {

    }

    const VariableT getNewRoot() const
    {
        return rootCalculatorAfter.result();
    }

    void generate_r1cs_witness(const Proof& _proof)
    {
        leafBefore.generate_r1cs_witness();
        leafAfter.generate_r1cs_witness();

        proof.fill_with_field_elements(pb, _proof.data);
        proofVerifierBefore.generate_r1cs_witness();
        rootCalculatorAfter.generate_r1cs_witness();
    }

    void generate_r1cs_constraints()
    {
        leafBefore.generate_r1cs_constraints();
        leafAfter.generate_r1cs_constraints();

        proofVerifierBefore.generate_r1cs_constraints();
        rootCalculatorAfter.generate_r1cs_constraints();
    }
};

class TradeHistoryTrimmingGadget : public GadgetT
{
public:

    const Constants& constants;

    VariableT tradeHistoryFilled;
    VariableT tradeHistoryCancelled;
    VariableT tradeHistoryOrderID;
    VariableT orderID;

    LeqGadget bNew;
    NotGadget bTrim;

    TernaryGadget filled;
    TernaryGadget cancelledToStore;
    TernaryGadget cancelled;
    TernaryGadget orderIDToStore;

    TradeHistoryTrimmingGadget(
        ProtoboardT& pb,
        const Constants& _constants,
        const VariableT& _tradeHistoryFilled,
        const VariableT& _tradeHistoryCancelled,
        const VariableT& _tradeHistoryOrderID,
        const VariableT& _orderID,
        const std::string& prefix
    ) :
        GadgetT(pb, prefix),

        constants(_constants),

        tradeHistoryFilled(_tradeHistoryFilled),
        tradeHistoryCancelled(_tradeHistoryCancelled),
        tradeHistoryOrderID(_tradeHistoryOrderID),

        orderID(_orderID),

        bNew(pb, tradeHistoryOrderID, orderID, NUM_BITS_ORDERID, FMT(prefix, ".tradeHistoryOrderID <(=) orderID")),
        bTrim(pb, bNew.leq(), FMT(prefix, ".!bNew")),

        filled(pb, bNew.lt(), constants.zero, tradeHistoryFilled, FMT(prefix, ".filled")),
        cancelledToStore(pb, bNew.lt(), constants.zero, tradeHistoryCancelled, FMT(prefix, ".cancelledToStore")),
        cancelled(pb, bTrim.result(), constants.one, cancelledToStore.result(), FMT(prefix, ".cancelled")),
        orderIDToStore(pb, bNew.lt(), orderID, tradeHistoryOrderID, FMT(prefix, ".orderIDToStore"))
    {

    }

    const VariableT& getFilled() const
    {
        return filled.result();
    }

    const VariableT& getCancelled() const
    {
        return cancelled.result();
    }

    const VariableT& getCancelledToStore() const
    {
        return cancelledToStore.result();
    }

    const VariableT& getOrderIDToStore() const
    {
        return orderIDToStore.result();
    }

    void generate_r1cs_witness()
    {
        bNew.generate_r1cs_witness();
        bTrim.generate_r1cs_witness();

        filled.generate_r1cs_witness();
        cancelledToStore.generate_r1cs_witness();
        cancelled.generate_r1cs_witness();
        orderIDToStore.generate_r1cs_witness();

        /*print(pb, "bNew", bNew.lt());
        print(pb, "bTrim", bTrim.result());
        print(pb, "filled", filled.result());
        print(pb, "cancelledToStore", cancelledToStore.result());
        print(pb, "cancelled", cancelled.result());
        print(pb, "orderIDToStore", orderIDToStore.result());*/
    }

    void generate_r1cs_constraints()
    {
        bNew.generate_r1cs_constraints();
        bTrim.generate_r1cs_constraints();

        filled.generate_r1cs_constraints();
        cancelledToStore.generate_r1cs_constraints();
        cancelled.generate_r1cs_constraints();
        orderIDToStore.generate_r1cs_constraints();
    }
};

}

#endif
