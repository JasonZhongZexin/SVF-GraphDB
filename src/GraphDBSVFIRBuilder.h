#ifndef GRAPHDBSVFIRBUILDER_H_
#define GRAPHDBSVFIRBUILDER_H_

#include "SVFIR/SVFIR.h"
#include "SVF-LLVM/BasicTypes.h"
#include "SVF-LLVM/ICFGBuilder.h"
#include "SVF-LLVM/LLVMModule.h"
#include "SVF-LLVM/LLVMUtil.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "Util/CallGraphBuilder.h"
#include "SVF-LLVM/LLVMLoopAnalysis.h"
#include "SVF-LLVM/CHGBuilder.h"
#include "GraphDBClient.h"


using namespace SVF;

namespace SVF{
class GraphDBSVFIRBuilder : public SVFIRBuilder
{

    public:
    lgraph::RpcClient* dbConnection = SVF::GraphDBClient::getInstance().getConnection();
    GraphDBSVFIRBuilder() = default;
    ~GraphDBSVFIRBuilder() = default;

    // override build()
    SVFIR* build() override
    {
        double startTime = SVFStat::getClk(true);

        DBOUT(DGENERAL, outs() << pasMsg("\t Building SVFIR ...\n"));

        if (Options::ReadFromDB())
        {
            GraphDBClient::getInstance().readSVFTypesFromDB(dbConnection, "SVFType", pag);
            GraphDBClient::getInstance().initialSVFPAGNodesFromDB(dbConnection, "PAG",pag);
            GraphDBClient::getInstance().readBasicBlockGraphFromDB(dbConnection, "BasicBlockGraph");
            CHGraph* chg = GraphDBClient::getInstance().buildCHGraphFromDB(dbConnection, "CHG", pag);
            pag->setCHG(chg);
            ICFG* icfg = GraphDBClient::getInstance().buildICFGFromDB(dbConnection, "ICFG", pag);
            pag->icfg = icfg;
            CallGraph* callGraph = GraphDBClient::getInstance().buildCallGraphFromDB(dbConnection,"CallGraph",pag);
            pag->callGraph = callGraph;
            GraphDBClient::getInstance().updatePAGNodesFromDB(dbConnection, "PAG", pag);
            GraphDBClient::getInstance().loadSVFPAGEdgesFromDB(dbConnection, "PAG",pag);
            GraphDBClient::getInstance().parseSVFStmtsForICFGNodeFromDBResult(pag);
            return pag;
        }

        // If the SVFIR has been built before, then we return the unique SVFIR of the program
        if (pag->getNodeNumAfterPAGBuild() > 1)
            return pag;

        createFunObjVars();

        /// build icfg
        ICFGBuilder icfgbuilder;
        pag->setICFG(icfgbuilder.build());

        /// initial external library information
        /// initial SVFIR nodes
        initialiseNodes();
        /// initial SVFIR edges:
        ///// handle globals
        visitGlobal();
        ///// collect exception vals in the program

        /// build callgraph
        CallGraphBuilder callGraphBuilder;
        std::vector<const FunObjVar *> funset;
        for (const auto &item : llvmModuleSet()->getFunctionSet())
        {
            funset.push_back(llvmModuleSet()->getFunObjVar(item));
        }
        pag->setCallGraph(callGraphBuilder.buildSVFIRCallGraph(funset));

        CHGraph *chg = new CHGraph();
        CHGBuilder chgbuilder(chg);
        chgbuilder.buildCHG();
        pag->setCHG(chg);

        /// handle functions
        for (Module &M : llvmModuleSet()->getLLVMModules())
        {
            for (Module::const_iterator F = M.begin(), E = M.end(); F != E; ++F)
            {
                const Function &fun = *F;
                const FunObjVar *svffun = llvmModuleSet()->getFunObjVar(&fun);
                /// collect return node of function fun
                if (!fun.isDeclaration())
                {
                    /// Return SVFIR node will not be created for function which can not
                    /// reach the return instruction due to call to abort(), exit(),
                    /// etc. In 176.gcc of SPEC 2000, function build_objc_string() from
                    /// c-lang.c shows an example when fun.doesNotReturn() evaluates
                    /// to TRUE because of abort().
                    if (fun.doesNotReturn() == false &&
                        fun.getReturnType()->isVoidTy() == false)
                    {
                        pag->addFunRet(svffun,
                                       pag->getGNode(pag->getReturnNode(svffun)));
                    }

                    /// To be noted, we do not record arguments which are in declared function without body
                    /// TODO: what about external functions with SVFIR imported by commandline?
                    for (Function::const_arg_iterator I = fun.arg_begin(), E = fun.arg_end();
                         I != E; ++I)
                    {
                        setCurrentLocation(&*I, &fun.getEntryBlock());
                        NodeID argValNodeId = llvmModuleSet()->getValueNode(&*I);
                        // if this is the function does not have caller (e.g. main)
                        // or a dead function, shall we create a black hole address edge for it?
                        // it is (1) too conservative, and (2) make FormalParmVFGNode defined at blackhole address PAGEdge.
                        // if(SVFUtil::ArgInNoCallerFunction(&*I)) {
                        //    if(I->getType()->isPointerTy())
                        //        addBlackHoleAddrEdge(argValNodeId);
                        //}
                        pag->addFunArgs(svffun, pag->getGNode(argValNodeId));
                    }
                }
                for (Function::const_iterator bit = fun.begin(), ebit = fun.end();
                     bit != ebit; ++bit)
                {
                    const BasicBlock &bb = *bit;
                    for (BasicBlock::const_iterator it = bb.begin(), eit = bb.end();
                         it != eit; ++it)
                    {
                        const Instruction &inst = *it;
                        setCurrentLocation(&inst, &bb);
                        visit(const_cast<Instruction &>(inst));
                    }
                }
            }
        }

        sanityCheck();

        pag->initialiseCandidatePointers();

        pag->setNodeNumAfterPAGBuild(pag->getTotalNodeNum());

        if (Options::Write2DB())
        {
            std::string dbname = "SVFType";
            GraphDBClient::getInstance().insertSVFTypeNodeSet2db(&pag->getSVFTypes(), &pag->getStInfos(), dbname);
            GraphDBClient::getInstance().insertPAG2db(pag);
            GraphDBClient::getInstance().insertICFG2db(pag->icfg);
            GraphDBClient::getInstance().insertCHG2db(chg);
            GraphDBClient::getInstance().insertCallGraph2db(pag->callGraph);
        }

        // dump SVFIR
        if (Options::PAGDotGraph())
            pag->dump("svfir_initial");

        // print to command line of the SVFIR graph
        if (Options::PAGPrint())
            pag->print();

        // dump ICFG
        if (Options::DumpICFG())
            pag->getICFG()->dump("icfg_initial");

        if (Options::LoopAnalysis())
        {
            LLVMLoopAnalysis loopAnalysis;
            loopAnalysis.build(pag->getICFG());
        }

        // dump SVFIR as JSON
        if (!Options::DumpJson().empty())
        {
            assert(false && "please implement SVFIRWriter::writeJsonToPath");
        }

        double endTime = SVFStat::getClk(true);
        SVFStat::timeOfBuildingSVFIR = (endTime - startTime) / TIMEINTERVAL;

        return pag;
    }
};
}
#endif // GRAPHDBSVFIRBUILDER_H_
