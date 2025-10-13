#include "SVF-LLVM/SVFIRBuilder.h"
#include "GraphDBClient.h"

class GraphDBSVFIRBuilder : public SVFIRBuilder
{
    public:
    SVFIR* build() override{
        double startTime = SVFStat::getClk(true);

        DBOUT(DGENERAL, outs() << pasMsg("\t Building SVFIR ...\n"));

        // We read SVFIR from a user-defined txt instead of parsing SVFIR from LLVM IR
        if (SVFIR::pagReadFromTXT())
        {
            PAGBuilderFromFile fileBuilder(SVFIR::pagFileName());
            return fileBuilder.build();
        }

        if (Options::ReadFromDB())
    {
            GraphDBClient::getInstance().readSVFTypesFromDB(dbConnection, "SVFType", pag);
            GraphDBClient::getInstance().initialSVFPAGNodesFromDB(dbConnection, "PAG",pag);
            GraphDBClient::getInstance().readBasicBlockGraphFromDB(dbConnection, "BasicBlockGraph");
            ICFG* icfg = GraphDBClient::getInstance().buildICFGFromDB(dbConnection, "ICFG", pag);
            pag->icfg = icfg;
            CallGraph* callGraph = GraphDBClient::getInstance().buildCallGraphFromDB(dbConnection,"CallGraph",pag);
            CHGraph* chg = new CHGraph();
            pag->setCHG(chg);
            pag->callGraph = callGraph;
            GraphDBClient::getInstance().updatePAGNodesFromDB(dbConnection, "PAG", pag);
            GraphDBClient::getInstance().loadSVFPAGEdgesFromDB(dbConnection, "PAG",pag);
            GraphDBClient::getInstance().parseSVFStmtsForICFGNodeFromDBResult(pag);
        return pag;
    }
}
}