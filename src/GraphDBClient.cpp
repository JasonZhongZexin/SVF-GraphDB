#include "SVFIR/GraphDBClient.h"
#include "SVFIR/SVFVariables.h"

using namespace SVF;

Map<int,FunObjVar*> id2funObjVarsMap;
Set<SVFBasicBlock*> basicBlocks;
Map<int, RetICFGNode*> id2RetICFGNodeMap;
Map<int, CallPE*> id2CallPEMap;
Map<int, RetPE*> id2RetPEMap;
Map<CallCFGEdge*, std::string> callCFGEdge2CallPEStrMap;
Map<RetCFGEdge*, int> retCFGEdge2RetPEStrMap;
Map<ICFGNode*, std::string> icfgNode2StmtsStrMap;
Map<int, SVFStmt*> edgeId2SVFStmtMap;
Map<SVFBasicBlock*, std::string> bb2AllICFGNodeIdstrMap;

bool GraphDBClient::loadSchema(lgraph::RpcClient* connection,
                               const std::string& filepath,
                               const std::string& dbname)
{
    if (nullptr != connection)
    {
        SVFUtil::outs() << "load schema from file:" << filepath << "\n";
        std::string result;
        bool ret = connection->ImportSchemaFromFile(result, filepath, dbname);
        if (!ret)
        {
            SVFUtil::outs() << dbname<< "Warining: Schema load failed:" << result << "\n";
        }
        return ret;
    }
    return false;
}

// create a new graph name CallGraph in db
bool GraphDBClient::createSubGraph(lgraph::RpcClient* connection, const std::string& graphname)
{
     ///TODO: graph name should be configurable
    if (nullptr != connection)
    {
        std::string result;
        connection->CallCypher(result, "CALL dbms.graph.deleteGraph('"+graphname+"')");
        bool ret = connection->CallCypherToLeader(result, "CALL dbms.graph.createGraph('"+graphname+"')");
        if (!ret)
        {
            SVFUtil::outs()
                << "Warining: Failed to create Graph callGraph:" << result << "\n";
        }
    }
    return false;
}

bool GraphDBClient::addICFGEdge2db(lgraph::RpcClient* connection,
                                   const ICFGEdge* edge,
                                   const std::string& dbname)
{
    if (nullptr != connection)
    {
        std::string queryStatement;
        if(const IntraCFGEdge* cfgEdge = SVFUtil::dyn_cast<IntraCFGEdge>(edge))
        {
            queryStatement = intraCFGEdge2DBString(cfgEdge);
        }
        else if (const CallCFGEdge* cfgEdge = SVFUtil::dyn_cast<CallCFGEdge>(edge))
        {
            queryStatement = callCFGEdge2DBString(cfgEdge);
        }
        else if (const RetCFGEdge* cfgEdge = SVFUtil::dyn_cast<RetCFGEdge>(edge))
        {
            queryStatement = retCFGEdge2DBString(cfgEdge);
        }
        else 
        {
            assert("unknown icfg edge type?");
            return false;
        }
        // SVFUtil::outs() << "ICFGEdge Query Statement:" << queryStatement << "\n";
        std::string result;
        if (queryStatement.empty())
        {
            return false;
        }
        bool ret = connection->CallCypher(result, queryStatement, dbname);
        if (!ret)
        {
            SVFUtil::outs() << "Warining: Failed to add ICFG edge to db " << dbname << " "
                            << result << "\n";
        }
        return ret;

    }
    return false;
}

bool GraphDBClient::addICFGNode2db(lgraph::RpcClient* connection,
                                   const ICFGNode* node,
                                   const std::string& dbname)
{
    if (nullptr != connection)
    {
        std::string queryStatement;
        if(const GlobalICFGNode* globalICFGNode = SVFUtil::dyn_cast<GlobalICFGNode>(node))
        {
           queryStatement = globalICFGNode2DBString(globalICFGNode);
        }
        else if (const IntraICFGNode* intraICFGNode = SVFUtil::dyn_cast<IntraICFGNode>(node))
        {
            queryStatement = intraICFGNode2DBString(intraICFGNode);
        }
        else if (const FunEntryICFGNode* funEntryICFGNode = SVFUtil::dyn_cast<FunEntryICFGNode>(node))
        {
            queryStatement = funEntryICFGNode2DBString(funEntryICFGNode);
        }
        else if (const FunExitICFGNode* funExitICFGNode = SVFUtil::dyn_cast<FunExitICFGNode>(node))
        {
            queryStatement = funExitICFGNode2DBString(funExitICFGNode);
        }
        else if (const CallICFGNode* callICFGNode = SVFUtil::dyn_cast<CallICFGNode>(node))
        {
            queryStatement = callICFGNode2DBString(callICFGNode);
        }
        else if (const RetICFGNode* retICFGNode = SVFUtil::dyn_cast<RetICFGNode>(node))
        {
            queryStatement = retICFGNode2DBString(retICFGNode);
        }
        else 
        {
            assert("unknown icfg node type?");
            return false;
        }

        // SVFUtil::outs()<<"ICFGNode Insert Query:"<<queryStatement<<"\n";
        std::string result;
        if (queryStatement.empty())
        {
            return false;
        }
        bool ret = connection->CallCypher(result, queryStatement, dbname);
        if (!ret)
        {
            SVFUtil::outs() << "Warining: Failed to add icfg node to db " << dbname << " "
                            << result << "\n";
        }
        return ret;
    }
    return false;
}

bool GraphDBClient::addCallGraphNode2db(lgraph::RpcClient* connection,
                                        const CallGraphNode* node,
                                        const std::string& dbname)
{
    if (nullptr != connection)
    {
        const std::string queryStatement = callGraphNode2DBString(node);
        // SVFUtil::outs()<<"CallGraph Node Insert Query:"<<queryStatement<<"\n";
        std::string result;
        bool ret = connection->CallCypher(result, queryStatement, dbname);
        if (!ret)
        {
            SVFUtil::outs() << "Warining: Failed to add callGraph node to db " << dbname << " "
                            << result << "\n";
        }
        return ret;
    }
    return false;
}

bool GraphDBClient::addCallGraphEdge2db(lgraph::RpcClient* connection,
                                        const CallGraphEdge* edge,
                                        const std::string& dbname)
{
    if (nullptr != connection)
    {
        const std::string queryStatement = callGraphEdge2DBString(edge);
        // SVFUtil::outs() << "Call Graph Edge Insert Query:" << queryStatement << "\n";
        std::string result;
        bool ret = connection->CallCypher(result, queryStatement, dbname);
        if (!ret)
        {
            SVFUtil::outs() << "Warining: Failed to add callgraph edge to db " << dbname << " "
                            << result << "\n";
        }
        // SVFUtil::outs()<<"CallGraph Edge Insert Query:"<<queryStatement<<"\n";
        return ret;
    }
    return false;
}

// pasre the directcallsIds/indirectcallsIds string to vector
std::vector<int> GraphDBClient::stringToIds(const std::string& str)
{
    std::vector<int> ids;
    std::istringstream iss(str);
    std::string token;
    while (std::getline(iss, token, ','))
    {
        ids.push_back(std::stoi(token));
    }
    return ids;
}

std::string GraphDBClient::getICFGNodeKindString(const ICFGNode* node)
{
    if(SVFUtil::isa<GlobalICFGNode>(node))
    {
        return "GlobalICFGNode";
    }
    else if (SVFUtil::isa<FunEntryICFGNode>(node))
    {
        return "FunEntryICFGNode";
    }
    else if (SVFUtil::isa<FunExitICFGNode>(node))
    {
        return "FunExitICFGNode";
    }
    else if (SVFUtil::isa<CallICFGNode>(node))
    {
        return "CallICFGNode";
    }
    else if (SVFUtil::isa<RetICFGNode>(node))
    {
        return "RetICFGNode";
    }
    else if (SVFUtil::isa<InterICFGNode>(node))
    {
        return "InterICFGNode";
    }
    else if (SVFUtil::isa<IntraICFGNode>(node))
    {
        return "IntraICFGNode";
    }
    else 
    {
        assert("unknown icfg node type?");
        return "";
    }
}

void GraphDBClient::insertCHG2db(const CHGraph* chg)
{
    std::string chgNodePath =
        SVF_SOURCE_DIR "/svf/include/Graphs/DBSchema/CHGNodeSchema.json";
    std::string chgEdgePath =
        SVF_SOURCE_DIR "/svf/include/Graphs/DBSchema/CHGEdgeSchema.json";
    // add all CHG Node & Edge to DB
    if (nullptr != connection)
    {
        // create a new graph name CHG in db
        createSubGraph(connection, "CHG");
        // load schema for CHG
        loadSchema(connection, chgEdgePath.c_str(), "CHG");
        loadSchema(connection, chgNodePath.c_str(), "CHG");
        std::vector<const CHEdge*> edges;
        for (auto it = chg->begin(); it != chg->end(); ++it)
        {
            CHNode* node = it->second;
            insertCHNode2db(connection, node, "CHG");
            for (auto edgeIter = node->OutEdgeBegin();
                 edgeIter != node->OutEdgeEnd(); ++edgeIter)
            {
                CHEdge* edge = *edgeIter;
                edges.push_back(edge);
            }
        }
        for (const auto& edge : edges)
        {
            insertCHEdge2db(connection, edge, "CHG");
        }
        for (const auto& pair : chg->callNodeToClassesMap)
        {
            const ICFGNode* icfgNode = pair.first;
            Set<const CHNode*> nodes = pair.second;
            std::string chNodes = extractNodesIds(nodes);
            if (chNodes.size()>0)
                updateCHNodes2ICFGNode(connection, "ICFG", icfgNode->getId(), chNodes, "chnodes_ids");
        }
        for (const auto& pair : chg->callNodeToCHAVtblsMap)
        {
            const ICFGNode* icfgNode = pair.first;
            Set<const GlobalObjVar*> vtables = pair.second;
            std::string vtblsIds = extractNodesIds(vtables);
            if (vtblsIds.size()>0)
                updateCHNodes2ICFGNode(connection, "ICFG", icfgNode->getId(), vtblsIds, "cha_vtbls_ids");
        }
    }
}

void GraphDBClient::updateCHNodes2ICFGNode(lgraph::RpcClient* connection, const std::string& dbname, const int icfgId, const std::string& dataStr, const std::string& fieldName)
{
    if(nullptr != connection)
    {
        std::string queryStatement;
        std::string nodeUpdateStatement = "MATCH (n{id:"+std::to_string(icfgId)+"}) SET n."+fieldName+" ='"+ dataStr + "'";
        std::string result;
        bool ret = connection->CallCypher(result, nodeUpdateStatement, dbname);
        if (!ret)
        {
            SVFUtil::outs() << "Warining: Failed to update ICFG node in db " << dbname << " "
                            << result << "\n";
        }
    }
}

void GraphDBClient::updateCallNode2ClassesMap(const ICFGNode* icfgNode, Set<int> chNodeIds, CHGraph* chg)
{
    if (nullptr != icfgNode)
    {
        for(int id:chNodeIds)
        {
            CHNode* chNode = chg->getGNode(id);
            if (chNode)
            {
                chg->callNodeToClassesMap[icfgNode].insert(chNode);
            }
        }
    }
}

void GraphDBClient::updateCallNode2CHAVtblsMap(const ICFGNode* icfgNode, Set<int> VTableSetIds, SVFIR* pag)
{
    if (nullptr != icfgNode)
    {
        Set<const GlobalObjVar*> vtbls;
        for(int id:VTableSetIds)
        {
            const GlobalObjVar* var = SVFUtil::dyn_cast<GlobalObjVar>(pag->getGNode(id));
            if (nullptr != var)
            {
                vtbls.insert(var);
            }
        }
        if (vtbls.size() > 0)
        {
            SVFUtil::cast<CHGraph>(pag->getCHG())->callNodeToCHAVtblsMap[icfgNode] = vtbls;
        }
    }
}

void GraphDBClient::insertCHNode2db(lgraph::RpcClient* connection, const CHNode* node, const std::string& dbname)
{
    if (nullptr != connection)
    {
        std::string queryStatement;
        if (SVFUtil::isa<CHNode>(node))
        {
            queryStatement = getCHNodeInsertStmt(SVFUtil::cast<CHNode>(node));
        }
        else
        {
            assert("unknown CHG node type?");
            return ;
        }
        // SVFUtil::outs()<<"CHNode Insert Query:"<<queryStatement<<"\n";
        std::string result;
        bool ret = connection->CallCypher(result, queryStatement, dbname);
        if (!ret)
        {
            SVFUtil::outs() << "Warining: Failed to add CHG node to db " << dbname << " "
                            << result << "\n";
        }
    }
}

std::string GraphDBClient::getCHNodeInsertStmt(const CHNode* node)
{
    std::string vtable_id = ", vtable_id: -1";
    if (nullptr != node->getVTable())
    {
        vtable_id = ", vtable_id: " + std::to_string(node->getVTable()->getId());
    }
    const std::string queryStatement ="CREATE (n:CHNode {id: " + std::to_string(node->getId()) +
    ", class_name:'" + node->getName() + "'" +
    vtable_id +
    ", flags:" + std::to_string(node->getFlags()) +
    ", virtual_function_vectors:'" + extractFuncVectors2String(node->getVirtualFunctionVectors()) + "'" +
    + "})";
    return queryStatement;
}

std::string GraphDBClient::getCHEdgeInsertStmt(const CHEdge* edge)
{
    const std::string queryStatement = 
        "MATCH (n:CHNode{class_name:'"+edge->getSrcNode()->getName()+"'}), (m:CHNode{class_name:'"+edge->getDstNode()->getName()+"'}) WHERE n.class_name = '" +
        edge->getSrcNode()->getName() +
        "' AND m.class_name = '" + edge->getDstNode()->getName() +
        "' CREATE (n)-[r:CHEdge{edge_type:" + std::to_string(edge->getEdgeType()) +
        "}]->(m)";
    return queryStatement;
}

void GraphDBClient::insertCHEdge2db(lgraph::RpcClient* connection, const CHEdge* edge, const std::string& dbname)
{
    if (nullptr != connection)
    {
        std::string queryStatement;
        if (SVFUtil::isa<CHEdge>(edge))
        {
            queryStatement = getCHEdgeInsertStmt(SVFUtil::cast<CHEdge>(edge));
        }
        else
        {
            assert("unknown CHG edge type?");
            return ;
        }
        // SVFUtil::outs()<<"CHEdge Insert Query:"<<queryStatement<<"\n";
        std::string result;
        bool ret = connection->CallCypher(result, queryStatement, dbname);
        if (!ret)
        {
            SVFUtil::outs() << "Warining: Failed to add CHG edge to db " << dbname << " "
                            << result << "\n";
        }
    }
}

void GraphDBClient::insertICFG2db(const ICFG* icfg)
{
    // add all ICFG Node & Edge to DB
    if (nullptr != connection)
    {
        // create a new graph name ICFG in db
        createSubGraph(connection, "ICFG");
        // load schema for CallGraph
        std::string ICFGNodePath =
            SVF_SOURCE_DIR "/svf/include/Graphs/DBSchema/ICFGNodeSchema.json";
        std::string ICFGEdgePath =
            SVF_SOURCE_DIR "/svf/include/Graphs/DBSchema/ICFGEdgeSchema.json";
        loadSchema(connection, ICFGNodePath.c_str(), "ICFG");
        loadSchema(connection, ICFGEdgePath.c_str(), "ICFG");
        std::vector<const ICFGEdge*> edges;
        for (auto it = icfg->begin(); it != icfg->end(); ++it)
        {
            ICFGNode* node = it->second;
            addICFGNode2db(connection, node, "ICFG");
            for (auto edgeIter = node->OutEdgeBegin();
                 edgeIter != node->OutEdgeEnd(); ++edgeIter)
            {
                ICFGEdge* edge = *edgeIter;
                edges.push_back(edge);
            }
        }
        for (auto edge : edges)
        {
            addICFGEdge2db(connection, edge, "ICFG");
        }
    }
}

void GraphDBClient::insertCallGraph2db(const CallGraph* callGraph)
{

    std::string callGraphNodePath =
        SVF_SOURCE_DIR "/svf/include/Graphs/DBSchema/CallGraphNodeSchema.json";
    std::string callGraphEdgePath =
        SVF_SOURCE_DIR "/svf/include/Graphs/DBSchema/CallGraphEdgeSchema.json";
    // add all CallGraph Node & Edge to DB
    if (nullptr != connection)
    {
        // create a new graph name CallGraph in db
        createSubGraph(connection, "CallGraph");
        // load schema for CallGraph
        SVF::GraphDBClient::getInstance().loadSchema(
            connection,
            callGraphEdgePath,
            "CallGraph");
        SVF::GraphDBClient::getInstance().loadSchema(
            connection,
            callGraphNodePath,
            "CallGraph");
        std::vector<const CallGraphEdge*> edges;
        for (const auto& item : *callGraph)
        {
            const CallGraphNode* node = item.second;
            addCallGraphNode2db(connection, node, "CallGraph");
            for (CallGraphEdge::CallGraphEdgeSet::iterator iter =
                     node->OutEdgeBegin();
                 iter != node->OutEdgeEnd(); ++iter)
            {
                const CallGraphEdge* edge = *iter;
                edges.push_back(edge);
            }
        }
        for (const auto& edge : edges)
        {
            addCallGraphEdge2db(connection, edge, "CallGraph");
        }

        } else {
        SVFUtil::outs() << "No DB connection, skip inserting CallGraph to DB\n";
        }
}

void GraphDBClient::insertSVFTypeNodeSet2db(const Set<const SVFType*>* types, const Set<const StInfo*>* stInfos, std::string& dbname)
{
    if (nullptr != connection)
    {
        // create a new graph name SVFType in db
        createSubGraph(connection, "SVFType");
        // load schema for SVFType
        loadSchema(connection, SVF_SOURCE_DIR "/svf/include/Graphs/DBSchema/SVFTypeNodeSchema.json", "SVFType");
        
        // load & insert each svftype node to db
        for (const auto& ty : *types)
        {
            std::string queryStatement;
            if (const SVFPointerType* svfType = SVFUtil::dyn_cast<SVFPointerType>(ty))
            {
                queryStatement = SVFPointerType2DBString(svfType);
            } 
            else if (const SVFIntegerType* svfType = SVFUtil::dyn_cast<SVFIntegerType>(ty))
            {
                queryStatement = SVFIntegerType2DBString(svfType);
            }
            else if (const SVFFunctionType* svfType = SVFUtil::dyn_cast<SVFFunctionType>(ty))
            {
                queryStatement = SVFFunctionType2DBString(svfType);
            }
            else if (const SVFStructType* svfType = SVFUtil::dyn_cast<SVFStructType>(ty))
            {
                queryStatement = SVFStructType2DBString(svfType);
            }
            else if (const SVFArrayType* svfType = SVFUtil::dyn_cast<SVFArrayType>(ty))
            {
                queryStatement = SVFArrayType2DBString(svfType);
            }
            else if (const SVFOtherType* svfType = SVFUtil::dyn_cast<SVFOtherType>(ty))
            {
                queryStatement = SVFOtherType2DBString(svfType);
            }
            else 
            {
                assert("unknown SVF type?");
                return ;
            }
    
            // SVFUtil::outs()<<"SVFType Insert Query:"<<queryStatement<<"\n";
            std::string result;
            bool ret = connection->CallCypher(result, queryStatement, dbname);
            if (!ret)
            {
                SVFUtil::outs() << "Warining: Failed to add SVFType node to db " << dbname << " "
                                << result << "\n";
            }
        
        }

        // load & insert each stinfo node to db
        for(const StInfo* stInfo : *stInfos)
        {
            // insert stinfo node to db
            std::string queryStatement = stInfo2DBString(stInfo);
            // SVFUtil::outs()<<"StInfo Insert Query:"<<queryStatement<<"\n";
            std::string result;
            bool ret = connection->CallCypher(result, queryStatement, dbname);
            if (!ret)
            {
                SVFUtil::outs() << "Warining: Failed to add StInfo node to db " << dbname << " "
                                << result << "\n";
            }
        }
    }

}

void GraphDBClient::insertBasicBlockGraph2db(const BasicBlockGraph* bbGraph)
{
    if (nullptr != connection)
    {
        std::vector<const BasicBlockEdge*> edges;
        for (auto& bb: *bbGraph)
        {
            SVFBasicBlock* node = bb.second;
            insertBBNode2db(connection, node, "BasicBlockGraph");
            for (auto iter = node->OutEdgeBegin(); iter != node->OutEdgeEnd(); ++iter)
            {
                edges.push_back(*iter);
            }
        }
        for (const BasicBlockEdge* edge : edges)
        {
            insertBBEdge2db(connection, edge, "BasicBlockGraph");
        }
    }
}

void GraphDBClient::insertBBEdge2db(lgraph::RpcClient* connection, const BasicBlockEdge* edge, const std::string& dbname)
{
    if (nullptr != connection)
    {
        std::string queryStatement = bbEdge2DBString(edge);
        // SVFUtil::outs()<<"BBEdge Insert Query:"<<queryStatement<<"\n";
        std::string result;
        if (!queryStatement.empty())
        {
            bool ret = connection->CallCypher(result, queryStatement, dbname);
            if (!ret)
            {
                SVFUtil::outs() << "Warining: Failed to add BB edge to db " << dbname
                                << " " << result << "\n";
            }
        }
    }
}

void GraphDBClient::insertBBNode2db(lgraph::RpcClient* connection, const SVFBasicBlock* node, const std::string& dbname)
{
    if (nullptr != connection)
    {
        std::string queryStatement = bb2DBString(node);
        // SVFUtil::outs()<<"BBNode Insert Query:"<<queryStatement<<"\n";
        std::string result;
        if (!queryStatement.empty())
        {
            bool ret = connection->CallCypher(result, queryStatement, dbname);
            if (!ret)
            {
                SVFUtil::outs() << "Warining: Failed to add BB node to db " << dbname
                                << " " << result << "\n";
            }
        }
    }
}

void GraphDBClient::insertPAG2db(const PAG* pag)
{
    std::string pagNodePath =
        SVF_SOURCE_DIR "/svf/include/Graphs/DBSchema/PAGNodeSchema.json";
    std::string pagEdgePath =
        SVF_SOURCE_DIR "/svf/include/Graphs/DBSchema/PAGEdgeSchema.json";
    std::string bbNodePath =
        SVF_SOURCE_DIR "/svf/include/Graphs/DBSchema/BasicBlockNodeSchema.json";
    std::string bbEdgePath =
        SVF_SOURCE_DIR "/svf/include/Graphs/DBSchema/BasicBlockEdgeSchema.json";

    // add all PAG Node & Edge to DB
    if (nullptr != connection)
    {
        // create a new graph name PAG in db
        createSubGraph(connection, "PAG");
        // create a new graph name BasicBlockGraph in db
        createSubGraph(connection, "BasicBlockGraph");
        // load schema for PAG
        SVF::GraphDBClient::getInstance().loadSchema(connection, pagEdgePath,
                                                     "PAG");
        SVF::GraphDBClient::getInstance().loadSchema(connection, pagNodePath,
                                                     "PAG");
        // load schema for PAG
        SVF::GraphDBClient::getInstance().loadSchema(connection, bbEdgePath,
                                                     "BasicBlockGraph");
        SVF::GraphDBClient::getInstance().loadSchema(connection, bbNodePath,
                                                     "BasicBlockGraph");

        std::vector<const SVFStmt*> edges;
        for (auto it = pag->begin(); it != pag->end(); ++it)
        {
            SVFVar* node = it->second;
            insertPAGNode2db(connection, node, "PAG");
            for (auto edgeIter = node->OutEdgeBegin();
                 edgeIter != node->OutEdgeEnd(); ++edgeIter)
            {
                SVFStmt* edge = *edgeIter;
                edges.push_back(edge);
            }
        }
        for (auto edge : edges)
        {
            insertPAGEdge2db(connection, edge, "PAG");
        }
    }
    else
    {
        SVFUtil::outs() << "No DB connection, skip inserting CallGraph to DB\n";
    }
}

void GraphDBClient::insertPAGEdge2db(lgraph::RpcClient* connection, const SVFStmt* edge, const std::string& dbname)
{
    if (nullptr != connection)
    {
        std::string queryStatement = getPAGEdgeInsertStmt(edge);
        // SVFUtil::outs()<<"PAGEdge Insert Query:"<<queryStatement<<"\n";
        std::string result;
        if (!queryStatement.empty())
        {
            bool ret = connection->CallCypher(result, queryStatement, dbname);
            if (!ret)
            {
                SVFUtil::outs() << "Warining: Failed to add PAG edge to db " << dbname
                                << " " << result << "\n";
            }
        }
    }
}

std::string GraphDBClient::getPAGEdgeInsertStmt(const SVFStmt* edge)
{
    std::string queryStatement = "";
    if(const TDForkPE* svfStmt = SVFUtil::dyn_cast<TDForkPE>(edge))
    {
        queryStatement = TDForkPE2DBString(svfStmt);
    } 
    else if(const TDJoinPE* svfStmt = SVFUtil::dyn_cast<TDJoinPE>(edge))
    {
        queryStatement = TDJoinPE2DBString(svfStmt);
    }
    else if(const CallPE* svfStmt = SVFUtil::dyn_cast<CallPE>(edge))
    {
        queryStatement = CallPE2DBString(svfStmt);
    }
    else if(const RetPE* svfStmt = SVFUtil::dyn_cast<RetPE>(edge))
    {
        queryStatement = RetPE2DBString(svfStmt);
    }
    else if(const GepStmt* svfStmt = SVFUtil::dyn_cast<GepStmt>(edge))
    {
        queryStatement = GepStmt2DBString(svfStmt);
    }
    else if(const LoadStmt* svfStmt = SVFUtil::dyn_cast<LoadStmt>(edge))
    {
        queryStatement = LoadStmt2DBString(svfStmt);
    }
    else if(const StoreStmt* svfStmt = SVFUtil::dyn_cast<StoreStmt>(edge))
    {
        queryStatement = StoreStmt2DBString(svfStmt);
    }
    else if(const CopyStmt* svfStmt = SVFUtil::dyn_cast<CopyStmt>(edge))
    {
        queryStatement = CopyStmt2DBString(svfStmt);
    }
    else if(const AddrStmt* svfStmt = SVFUtil::dyn_cast<AddrStmt>(edge))
    {
        queryStatement = AddrStmt2DBString(svfStmt);
    }
    else if(const AssignStmt* svfStmt = SVFUtil::dyn_cast<AssignStmt>(edge))
    {
        queryStatement = AssignStmt2DBString(svfStmt);
    }
    else if(const PhiStmt* svfStmt = SVFUtil::dyn_cast<PhiStmt>(edge))
    {
        queryStatement = PhiStmt2DBString(svfStmt);
    }
    else if(const SelectStmt* svfStmt = SVFUtil::dyn_cast<SelectStmt>(edge))
    {
        queryStatement = SelectStmt2DBString(svfStmt);
    }
    else if(const CmpStmt* svfStmt = SVFUtil::dyn_cast<CmpStmt>(edge))
    {
        queryStatement = CmpStmt2DBString(svfStmt);
    }
    else if(const BinaryOPStmt* svfStmt = SVFUtil::dyn_cast<BinaryOPStmt>(edge))
    {
        queryStatement = BinaryOPStmt2DBString(svfStmt);
    }
    else if(const MultiOpndStmt* svfStmt = SVFUtil::dyn_cast<MultiOpndStmt>(edge))
    {
        queryStatement = MultiOpndStmt2DBString(svfStmt);
    }
    else if(const UnaryOPStmt* svfStmt = SVFUtil::dyn_cast<UnaryOPStmt>(edge))
    {
        queryStatement = UnaryOPStmt2DBString(svfStmt);
    }
    else if(const BranchStmt* svfStmt = SVFUtil::dyn_cast<BranchStmt>(edge))
    {
        queryStatement = BranchStmt2DBString(svfStmt);
    }
    else if(const SVFStmt* svfStmt = SVFUtil::dyn_cast<SVFStmt>(edge))
    {
        queryStatement = SVFStmt2DBString(svfStmt);
    }
    else
    {
        assert("unknown SVFStmt type?");
    }
    return queryStatement;
}

void GraphDBClient::insertPAGNode2db(lgraph::RpcClient* connection, const SVFVar* node, const std::string& dbname)
{
    if (nullptr != connection)
    {
        std::string queryStatement = getPAGNodeInsertStmt(node);
        // SVFUtil::outs()<<"PAGNode Insert Query:"<<queryStatement<<"\n";
        std::string result;
        if (!queryStatement.empty())
        {
            bool ret = connection->CallCypher(result, queryStatement, dbname);
            if (!ret)
            {
                SVFUtil::outs() << "Warining: Failed to add PAG node to db " << dbname
                                << " " << result << "\n";
            }
        }
    }
}

std::string GraphDBClient::getPAGNodeInsertStmt(const SVFVar* node)
{
    std::string queryStatement = "";
    if(const ConstNullPtrValVar* svfVar = SVFUtil::dyn_cast<ConstNullPtrValVar>(node))
    {
        queryStatement = constNullPtrValVar2DBString(svfVar);
    }
    else if(const ConstIntValVar* svfVar = SVFUtil::dyn_cast<ConstIntValVar>(node))
    {
        queryStatement = constIntValVar2DBString(svfVar);
    }
    else if(const ConstFPValVar* svfVar = SVFUtil::dyn_cast<ConstFPValVar>(node))
    {
        queryStatement = constFPValVar2DBString(svfVar);
    }
    else if(const BlackHoleValVar* svfVar = SVFUtil::dyn_cast<BlackHoleValVar>(node))
    {
        queryStatement = blackHoleValVar2DBString(svfVar);
    }
    else if(const ConstDataValVar* svfVar = SVFUtil::dyn_cast<ConstDataValVar>(node))
    {
        queryStatement = constDataValVar2DBString(svfVar);
    }
    else if(const RetValPN* svfVar = SVFUtil::dyn_cast<RetValPN>(node))
    {
        queryStatement = retValPN2DBString(svfVar);
    }
    else if(const VarArgValPN* svfVar = SVFUtil::dyn_cast<VarArgValPN>(node))
    {
        queryStatement = varArgValPN2DBString(svfVar);
    }
    else if(const DummyValVar* svfVar = SVFUtil::dyn_cast<DummyValVar>(node))
    {
        queryStatement = dummyValVar2DBString(svfVar);
    }
    else if(const ConstAggValVar* svfVar = SVFUtil::dyn_cast<ConstAggValVar>(node))
    {
        queryStatement = constAggValVar2DBString(svfVar);
    }
    else if(const GlobalValVar* svfVar = SVFUtil::dyn_cast<GlobalValVar>(node))
    {
        queryStatement = globalValVar2DBString(svfVar);
    }
    else if(const FunValVar* svfVar = SVFUtil::dyn_cast<FunValVar>(node))
    {
        queryStatement = funValVar2DBString(svfVar);
    }
    else if(const GepValVar* svfVar = SVFUtil::dyn_cast<GepValVar>(node))
    {
        queryStatement = gepValVar2DBString(svfVar);
    }
    else if(const ArgValVar* svfVar = SVFUtil::dyn_cast<ArgValVar>(node))
    {
        queryStatement = argValVar2DBString(svfVar);
    }
    else if(const ValVar* svfVar = SVFUtil::dyn_cast<ValVar>(node))
    {
        queryStatement = valVar2DBString(svfVar);
    }
    else if(const ConstNullPtrObjVar* svfVar = SVFUtil::dyn_cast<ConstNullPtrObjVar>(node))
    {
        queryStatement = constNullPtrObjVar2DBString(svfVar);
    }
    else if(const ConstIntObjVar* svfVar = SVFUtil::dyn_cast<ConstIntObjVar>(node))
    {
        queryStatement = constIntObjVar2DBString(svfVar);
    }
    else if(const ConstFPObjVar* svfVar = SVFUtil::dyn_cast<ConstFPObjVar>(node))
    {
        queryStatement = constFPObjVar2DBString(svfVar);
    }
    else if(const ConstDataObjVar* svfVar = SVFUtil::dyn_cast<ConstDataObjVar>(node))
    {
        queryStatement = constDataObjVar2DBString(svfVar);
    }
    else if(const DummyObjVar* svfVar = SVFUtil::dyn_cast<DummyObjVar>(node))
    {
        queryStatement = dummyObjVar2DBString(svfVar);
    }
    else if(const ConstAggObjVar* svfVar = SVFUtil::dyn_cast<ConstAggObjVar>(node))
    {
        queryStatement = constAggObjVar2DBString(svfVar);
    }
    else if(const GlobalObjVar* svfVar = SVFUtil::dyn_cast<GlobalObjVar>(node))
    {
        queryStatement = globalObjVar2DBString(svfVar);
    }
    else if(const FunObjVar* svfVar = SVFUtil::dyn_cast<FunObjVar>(node))
    {
        queryStatement = funObjVar2DBString(svfVar);
        if ( nullptr != svfVar->getBasicBlockGraph())
        {
            insertBasicBlockGraph2db(svfVar->getBasicBlockGraph());
        }
    }
    else if(const StackObjVar* svfVar = SVFUtil::dyn_cast<StackObjVar>(node))
    {
        queryStatement = stackObjVar2DBString(svfVar);
    }
    else if(const HeapObjVar* svfVar = SVFUtil::dyn_cast<HeapObjVar>(node))
    {
        queryStatement = heapObjVar2DBString(svfVar);
    } 
    else if(const BaseObjVar* svfVar = SVFUtil::dyn_cast<BaseObjVar>(node))
    {
        queryStatement = baseObjVar2DBString(svfVar);
    }
    else if(const GepObjVar* svfVar = SVFUtil::dyn_cast<GepObjVar>(node))
    {
        queryStatement = gepObjVar2DBString(svfVar);
    }
    else if(const ObjVar* svfVar = SVFUtil::dyn_cast<ObjVar>(node))
    {
        queryStatement = objVar2DBString(svfVar);
    }
    else
    {
        assert("unknown SVFVar type?");
    }
    return queryStatement;
}

std::string GraphDBClient::getPAGNodeKindString(const SVFVar* node)
{
    if(SVFUtil::isa<ConstNullPtrValVar>(node))
    {
        return "ConstNullPtrValVar";
    }
    else if(SVFUtil::isa<ConstIntValVar>(node))
    {
        return "ConstIntValVar";
    }
    else if(SVFUtil::isa<ConstFPValVar>(node))
    {
        return "ConstFPValVar";
    }
    else if(SVFUtil::isa<BlackHoleValVar>(node))
    {
        return "BlackHoleValVar";
    }
    else if(SVFUtil::isa<ConstDataValVar>(node))
    {
        return "ConstDataValVar";
    }
    else if(SVFUtil::isa<RetValPN>(node))
    {
        return "RetValPN";
    }
    else if(SVFUtil::isa<VarArgValPN>(node))
    {
        return "VarArgValPN";
    }
    else if(SVFUtil::isa<DummyValVar>(node))
    {
        return "DummyValVar";
    }
    else if(SVFUtil::isa<ConstAggValVar>(node))
    {
        return "ConstAggValVar";
    }
    else if(SVFUtil::isa<GlobalValVar>(node))
    {
        return "GlobalValVar";
    }
    else if(SVFUtil::isa<FunValVar>(node))
    {
        return "FunValVar";
    }
    else if(SVFUtil::isa<GepValVar>(node))
    {
        return "GepValVar";
    }
    else if(SVFUtil::isa<ArgValVar>(node))
    {
        return "ArgValVar";
    }
    else if(SVFUtil::isa<ValVar>(node))
    {
        return "ValVar";
    }
    else if(SVFUtil::isa<ConstNullPtrObjVar>(node))
    {
        return "ConstNullPtrObjVar";
    }
    else if(SVFUtil::isa<ConstIntObjVar>(node))
    {
        return "ConstIntObjVar";
    }
    else if(SVFUtil::isa<ConstFPObjVar>(node))
    {
        return "ConstFPObjVar";
    }
    else if(SVFUtil::isa<ConstDataObjVar>(node))
    {
        return "ConstDataObjVar";
    }
    else if(SVFUtil::isa<DummyObjVar>(node))
    {
        return "DummyObjVar";
    }
    else if(SVFUtil::isa<ConstAggObjVar>(node))
    {
        return "ConstAggObjVar";
    }
    else if(SVFUtil::isa<GlobalObjVar>(node))
    {
        return "GlobalObjVar";
    }
    else if(SVFUtil::isa<FunObjVar>(node))
    {
        return "FunObjVar";
    }
    else if(SVFUtil::isa<StackObjVar>(node))
    {
        return "StackObjVar";
    }
    else if(SVFUtil::isa<HeapObjVar>(node))
    {
        return "HeapObjVar";
    } 
    else if(SVFUtil::isa<BaseObjVar>(node))
    {
        return "BaseObjVar";
    }
    else if(SVFUtil::isa<GepObjVar>(node))
    {
        return "GepObjVar";
    }
    else if(SVFUtil::isa<ObjVar>(node))
    {
        return "ObjVar";
    }
   
        assert("unknown SVFVar node type?");
        return "SVFVar";
    
}

void GraphDBClient::readSVFTypesFromDB(lgraph::RpcClient* connection, const std::string& dbname, SVFIR* pag)
{
    SVFUtil::outs()<< "Build SVF types from DB....\n";
    addSVFTypeNodeFromDB(connection, dbname, pag);
}

void GraphDBClient::addSVFTypeNodeFromDB(lgraph::RpcClient* connection, const std::string& dbname, SVFIR* pag)
{
    // parse all SVFType
    std::string queryStatement = "MATCH (node) WHERE NOT 'StInfo' IN labels(node) return node";

    Map<int, SVFType*> svfTypeMap;
    Map<int, StInfo*> stInfoMap;
    // Map<SVFType::SVFTyKind, Set<SVFType*>> svfTypeKind2SVFTypesMap;
    Map<SVFType*, std::pair<int, int>> svfi8AndPtrTypeMap;
    Map<int, Set<SVFFunctionType*>> functionRetTypeSetMap;
    Map<SVFFunctionType*, std::vector<int>> functionParamsTypeSetMap;
    Map<int,Set<SVFType*>> stInfoId2SVFTypeMap;
    Map<int, Set<SVFArrayType*>> elementTyepsMap;
    Map<SVFStructType*, std::vector<int>> structType2FieldsTypeIdMap;
    
    cJSON* root = queryFromDB(connection, dbname, queryStatement);
    cJSON* node;
    if (nullptr != root)
    {
        cJSON_ArrayForEach(node, root)
        {
            cJSON* data = cJSON_GetObjectItem(node, "node");
            if (!data)
                continue;

            cJSON* properties = cJSON_GetObjectItem(data, "properties");
            if (!properties)
                continue;

            std::string label = cJSON_GetObjectItem(data, "label")->valuestring;

            SVFType* type = nullptr;
            int i8Type =
                cJSON_GetObjectItem(properties, "svf_i8_type_id")->valueint;
            int ptrType =
                cJSON_GetObjectItem(properties, "svf_ptr_type_id")->valueint;
            bool svt = cJSON_IsTrue(
                cJSON_GetObjectItem(properties, "is_single_val_ty"));
            int byteSize =
                cJSON_GetObjectItem(properties, "byte_size")->valueint;
            int typeId =
                cJSON_GetObjectItem(properties, "id")->valueint;

            if (label == "SVFPointerType")
            {
                type = new SVFPointerType(typeId, byteSize, svt);
            }
            else if (label == "SVFIntegerType")
            {
                cJSON* single_and_width_Json =
                    cJSON_GetObjectItem(properties, "single_and_width");
                short single_and_width =
                    (short)cJSON_GetNumberValue(single_and_width_Json);
                type = new SVFIntegerType(typeId, byteSize, svt, single_and_width);
            }
            else if (label == "SVFFunctionType")
            {
                SVFFunctionType* funType = new SVFFunctionType(typeId, svt, byteSize);
                type = funType;
                int retTypeId = cJSON_GetObjectItem(properties, "ret_ty_node_id") ->valueint;
                auto it = svfTypeMap.find(retTypeId);
                if (it != svfTypeMap.end())
                {
                    funType->setReturnType(it->second);
                }
                else
                {
                    functionRetTypeSetMap[retTypeId].insert(funType);
                }
                std::string paramsTypes = cJSON_GetObjectItem(properties, "params_types_vec")->valuestring;
                if (!paramsTypes.empty())
                {
                    functionParamsTypeSetMap[funType] = parseSVFTypes(paramsTypes);
                }
            }
            else if (label == "SVFOtherType")
            {
                std::string repr =
                    cJSON_GetObjectItem(properties, "repr")->valuestring;
                type = new SVFOtherType(typeId, svt, byteSize, repr);
            }
            else if (label == "SVFStructType")
            {
                std::string name = cJSON_GetObjectItem(properties, "struct_name")->valuestring;
                SVFStructType* structType = new SVFStructType(typeId, svt, byteSize, name);
                type = structType;
                std::string fieldTypesStr = cJSON_GetObjectItem(properties, "fields_id_vec")->valuestring;
                if (!fieldTypesStr.empty())
                {
                    structType2FieldsTypeIdMap[structType] = parseSVFTypes(fieldTypesStr);
                }
                int stInfoID = cJSON_GetObjectItem(properties, "stinfo_node_id")->valueint;
                auto it = stInfoMap.find(stInfoID);
                if (it != stInfoMap.end())
                {
                    type->setTypeInfo(it->second);
                }
                else
                {
                    stInfoId2SVFTypeMap[stInfoID].insert(type);
                }
            }
            else if (label == "SVFArrayType")
            {
                int numOfElement = cJSON_GetObjectItem(properties, "num_of_element")->valueint;
                SVFArrayType* arrayType = new SVFArrayType(typeId, svt, byteSize, numOfElement);
                type = arrayType;
                int stInfoID = cJSON_GetObjectItem(properties, "stinfo_node_id")->valueint;
                auto stInfoIter = stInfoMap.find(stInfoID);
                if (stInfoIter != stInfoMap.end())
                {
                    type->setTypeInfo(stInfoIter->second);
                }
                else
                {
                    stInfoId2SVFTypeMap[stInfoID].insert(type);
                }
                int typeOfElementId = cJSON_GetObjectItem(properties,"type_of_element_node_type_id")->valueint;
                auto tyepIter = svfTypeMap.find(typeOfElementId);
                if (tyepIter != svfTypeMap.end())
                {
                    arrayType->setTypeOfElement(tyepIter->second);
                }
                else
                {
                    elementTyepsMap[typeOfElementId].insert(arrayType);
                }
            }
            svfTypeMap.emplace(typeId, type);
            // svfTypeKind2SVFTypesMap[type->getSVFTyKind()].insert(type);
            svfi8AndPtrTypeMap[type] = std::make_pair(i8Type, ptrType);
        }
        cJSON_Delete(root);
    }

    // parse all StInfo
    queryStatement = "MATCH (node:StInfo) return node";
    root = queryFromDB(connection, dbname, queryStatement);
    if (nullptr != root)
    {
        cJSON_ArrayForEach(node, root)
        {
            cJSON* data = cJSON_GetObjectItem(node, "node");
            if (!data)
                continue;

            cJSON* properties = cJSON_GetObjectItem(data, "properties");
            if (!properties)
                continue;

            u32_t id = static_cast<u32_t>(cJSON_GetObjectItem(properties, "st_info_id")->valueint);
            std::string fld_idx_vec = cJSON_GetObjectItem(properties, "fld_idx_vec")->valuestring;
            std::vector<u32_t> fldIdxVec = parseElements2Container<std::vector<u32_t>>(fld_idx_vec);

            std::string elem_idx_vec = cJSON_GetObjectItem(properties, "elem_idx_vec")->valuestring;
            std::vector<u32_t> elemIdxVec = parseElements2Container<std::vector<u32_t>>(elem_idx_vec);

            std::string fld_idx_2_type_map = cJSON_GetObjectItem(properties, "fld_idx_2_type_map")->valuestring;
            Map<u32_t, const SVFType*> fldIdx2TypeMap = parseStringToFldIdx2TypeMap<Map<u32_t, const SVFType*>>(fld_idx_2_type_map, svfTypeMap);

            std::string finfo_types = cJSON_GetObjectItem(properties, "finfo_types")->valuestring;
            std::vector<const SVFType*> finfo = parseElementsToSVFTypeContainer<std::vector<const SVFType*>>(finfo_types, svfTypeMap);

            u32_t stride = static_cast<u32_t>(cJSON_GetObjectItem(properties, "stride")->valueint);
            u32_t num_of_flatten_elements = static_cast<u32_t>(cJSON_GetObjectItem(properties, "num_of_flatten_elements")->valueint);
            u32_t num_of_flatten_fields = static_cast<u32_t>(cJSON_GetObjectItem(properties, "num_of_flatten_fields")->valueint);
            std::string flatten_element_types =cJSON_GetObjectItem(properties, "flatten_element_types")->valuestring;
            std::vector<const SVFType*> flattenElementTypes =parseElementsToSVFTypeContainer<std::vector<const SVFType*>>(flatten_element_types, svfTypeMap);
            StInfo* stInfo =new StInfo(id, fldIdxVec, elemIdxVec, fldIdx2TypeMap, finfo,stride, num_of_flatten_elements,num_of_flatten_fields, flattenElementTypes);
            stInfoMap[id] = stInfo;
        }
        cJSON_Delete(root);
    }

    for (auto& [retTypeId, types]:functionRetTypeSetMap)
    {
        auto retTypeIter = svfTypeMap.find(retTypeId);
        if (retTypeIter != svfTypeMap.end())
        {
            for (auto& type: types)
            {
                type->setReturnType(retTypeIter->second);
            }
        }
        else
        {
            SVFUtil::outs()
                << "Warning: No matching RetType found for typeId: " << retTypeId << "\n";
        }
    }

    for (auto& [funType, paramsVec]:functionParamsTypeSetMap)
    {
        for (const int param : paramsVec)
        {
            auto paramTypeIter = svfTypeMap.find(param);
            if (paramTypeIter != svfTypeMap.end())
            {
                funType->addParamType(paramTypeIter->second);
            } 
            else
            {
                SVFUtil::outs()<<"Warning: No matching paramType found for typeID: "
                              << param << "\n";
            }
        }
    }

    for (auto& [structType, fieldTypes]:structType2FieldsTypeIdMap)
    {
        for (const int fieldTypeId : fieldTypes)
        {
            auto fieldTypeIter = svfTypeMap.find(fieldTypeId);
            if (fieldTypeIter != svfTypeMap.end())
            {
                structType->addFieldsType(fieldTypeIter->second);
            }
            else
            {
                SVFUtil::outs()<<"Warning: No matching fieldType found for typeID: "
                              << fieldTypeId << "\n";
            }
        }
    }

    for (auto&[stInfoId, types] : stInfoId2SVFTypeMap)
    {
        auto stInfoIter = stInfoMap.find(stInfoId);
        if (stInfoIter != stInfoMap.end())
        {
            for (SVFType* type: types)
            {
                type->setTypeInfo(stInfoIter->second);
                if (SVFUtil::isa<SVFStructType>(type) && stInfoIter->second->getNumOfFlattenFields() > pag->maxStSize)
                {
                    pag->maxStSize = stInfoIter->second->getNumOfFlattenFields();
                    pag->maxStruct = type;
                }
            }
        }
        else
        {
            SVFUtil::outs()<<"Warning: No matching StInfo found for id: "
            << stInfoId << "\n";
        }
    }

    for (auto& [elementTypesId, arrayTypes]:elementTyepsMap)
    {
        auto elementTypeIter = svfTypeMap.find(elementTypesId);
        if (elementTypeIter != svfTypeMap.end())
        {
            for (SVFArrayType* type : arrayTypes)
            {
                type->setTypeOfElement(elementTypeIter->second);
            }
        }
        else 
        {
            SVFUtil::outs()<<"Warning: No matching elementType found for typeId: "
            << elementTypesId << "\n";
        }
    }

    for (auto& [svfType, pair]:svfi8AndPtrTypeMap)
    {
        int svfi8Type = pair.first;
        int svfptrType = pair.second;
        auto i8Type = svfTypeMap.find(svfi8Type);
        auto ptrType = svfTypeMap.find(svfptrType);
        if (i8Type!=svfTypeMap.end())
        {
            svfType->setSVFInt8Type(i8Type->second);
        }
        if (ptrType!= svfTypeMap.end())
        {
            svfType->setSVFPtrType(ptrType->second);
        }
    }
    for (auto& [typeId, type] : svfTypeMap)
    {
        pag->addTypeInfo(type);
    }
    for (auto& [id, stInfo]: stInfoMap)
    {
        pag->addStInfo(stInfo);
    }
}

void GraphDBClient::updateRetPE4RetCFGEdge()
{
    if (retCFGEdge2RetPEStrMap.size() > 0)
    {
        for (auto& [edge, id] : retCFGEdge2RetPEStrMap)
        {
            if (nullptr != edge && id != -1)
            {
                auto it = id2RetPEMap.find(id);
                if (it != id2RetPEMap.end())
                {
                    RetPE* retPE = it->second;
                    edge->addRetPE(retPE);
                }
                else
                {
                    SVFUtil::outs() << "Warning[updateRetPE4RetCFGEdge]: No matching RetPE found for id: " << id << "\n";
                }
            }
        }
    }
}

void GraphDBClient::updateCallPEs4CallCFGEdge()
{
    if (callCFGEdge2CallPEStrMap.size() > 0)
    {
        for (auto& [edge, ids] : callCFGEdge2CallPEStrMap)
        {
            if (nullptr != edge && !ids.empty())
            {
                std::vector<int> idVec = parseElements2Container<std::vector<int>>(ids);
                for (int id : idVec)
                {
                    auto it = id2CallPEMap.find(id);
                    if (it != id2CallPEMap.end())
                    {
                        CallPE* callPE = it->second;
                        edge->addCallPE(callPE);
                    }
                    else
                    {
                        SVFUtil::outs() << "Warning[updateCallPEs4CallCFGEdge]: No matching CallPE found for id: " << id << "\n";
                    }
                }
            }
        }
    }
}

void GraphDBClient::loadSVFPAGEdgesFromDB(lgraph::RpcClient* connection, const std::string& dbname, SVFIR* pag)
{
    SVFUtil::outs()<< "Loading SVF PAG edges from DB....\n";
    readPAGEdgesFromDB(connection, dbname, "AddrStmt", pag);
    readPAGEdgesFromDB(connection, dbname, "CopyStmt", pag);
    readPAGEdgesFromDB(connection, dbname, "StoreStmt", pag);
    readPAGEdgesFromDB(connection, dbname, "LoadStmt", pag);
    readPAGEdgesFromDB(connection, dbname, "GepStmt", pag);
    readPAGEdgesFromDB(connection, dbname, "CallPE", pag);
    readPAGEdgesFromDB(connection, dbname, "RetPE", pag);
    readPAGEdgesFromDB(connection, dbname, "PhiStmt", pag);
    readPAGEdgesFromDB(connection, dbname, "SelectStmt", pag);
    readPAGEdgesFromDB(connection, dbname, "CmpStmt", pag);
    readPAGEdgesFromDB(connection, dbname, "BinaryOPStmt", pag);
    readPAGEdgesFromDB(connection, dbname, "UnaryOPStmt", pag);
    readPAGEdgesFromDB(connection, dbname, "BranchStmt", pag);
    readPAGEdgesFromDB(connection, dbname, "TDForkPE", pag);
    readPAGEdgesFromDB(connection, dbname, "RetPETDJoinPE", pag);
    
    updateCallPEs4CallCFGEdge();
    updateRetPE4RetCFGEdge();
    SVFUtil::outs()<< "Loading SVF PAG edges from DB done....\n";

}

void GraphDBClient::readPAGEdgesFromDB(lgraph::RpcClient* connection, const std::string& dbname, std::string edgeType, SVFIR* pag)
{
    int skip = 0;
    int limit = 1000;
    while (true)
    {
        // SVFUtil::outs() << "Loading SVF PAG edges from DB, edge type: " << edgeType << ", skip: " << skip << ", limit: " << limit << "\n";
        std::string queryStatement =  "MATCH ()-[edge:"+edgeType+"]->() RETURN edge SKIP "+std::to_string(skip)+" LIMIT "+std::to_string(limit);
        cJSON* root = queryFromDB(connection, dbname, queryStatement);
        if ( nullptr == root)
        {
            break;
        }
        else
        {
            cJSON* edge;
            cJSON_ArrayForEach(edge, root)
            {
                cJSON* data = cJSON_GetObjectItem(edge, "edge");
                if (!data)
                    continue;
                cJSON* properties = cJSON_GetObjectItem(data, "properties");
                if (!properties)
                    continue;
    
                // parse src SVFVar & dst SVFVar
                int src_id = cJSON_GetObjectItem(data,"src")->valueint;
                int dst_id = cJSON_GetObjectItem(data,"dst")->valueint;
                SVFVar* srcNode = pag->getGNode(src_id);
                SVFVar* dstNode = pag->getGNode(dst_id);
                if (nullptr == srcNode)
                {
                    SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching src SVFVar found for id: " << src_id << "\n";
                    continue;
                }
                if( nullptr == dstNode)
                {
                    SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching dst SVFVar found for id: " << dst_id << "\n";
                    continue;
                }
    
                int edge_id = cJSON_GetObjectItem(properties,"edge_id")->valueint;
                int svf_var_node_id = cJSON_GetObjectItem(properties,"svf_var_node_id")->valueint; 
                SVFVar* value = nullptr;
                if (svf_var_node_id != -1)
                {
                    value = pag->getGNode(svf_var_node_id);
                }
                int icfg_node_id = cJSON_GetObjectItem(properties,"icfg_node_id")->valueint;
                ICFGNode* icfgNode = nullptr;
                if (icfg_node_id != -1)
                {
                    icfgNode = pag->getICFG()->getICFGNode(icfg_node_id);
                }
    
                std::string bb_id = cJSON_GetObjectItem(properties,"bb_id")->valuestring;
                SVFBasicBlock* bb = nullptr;
                if (!bb_id.empty())
                {
                    std::pair<int, int> pair = parseBBIdPair(bb_id);
                    if (pair.first != -1 && pair.second != -1)
                    {
                        FunObjVar* fun = SVFUtil::dyn_cast<FunObjVar>(pag->getGNode(pair.first));
                        if (nullptr != fun)
                        {
                            bb = fun->getBasicBlockGraph()->getGNode(pair.second);
                            if (nullptr == bb)
                            {
                                SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching BB found for id: " << bb_id << "\n";
                                continue;
                            }
                        }
                    }
                }
    
                int call_edge_label_counter = cJSON_GetObjectItem(properties,"call_edge_label_counter")->valueint; 
                int store_edge_label_counter = cJSON_GetObjectItem(properties,"store_edge_label_counter")->valueint; 
                int multi_opnd_label_counter = cJSON_GetObjectItem(properties,"multi_opnd_label_counter")->valueint; 
                s64_t edgeFlag = static_cast<u64_t>(cJSON_GetObjectItem(properties,"edge_flag")->valueint);
    
                SVFStmt* stmt = nullptr;
    
                if (edgeType == "AddrStmt")
                {
                    stmt = new AddrStmt(srcNode, dstNode, edgeFlag, edge_id, value, icfgNode);
                    std::string arr_size = cJSON_GetObjectItem(properties,"arr_size")->valuestring;
                    AddrStmt* addrStmt = SVFUtil::cast<AddrStmt>(stmt);
                    if (!arr_size.empty())
                    {
                        Set<int> arrSizeVec = parseElements2Container<Set<int>>(arr_size);
                        for (int varId : arrSizeVec)
                        {
                            SVFVar* var = pag->getGNode(varId);
                            if (nullptr != var)
                            {
                                addrStmt->addArrSize(var);
                            }
                            else 
                            {
                                SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching SVFVar found for id: " << varId << " when processing AddrStmt:"<<edge_id<<"\n";
                            }
                        }
                    }
                    if (!pag->hasEdge(addrStmt, SVFStmt::Addr))
                    {
                        pag->addAddrStmt(addrStmt);
                    }
                }
                else if (edgeType == "CopyStmt")
                {
                    int copy_kind = cJSON_GetObjectItem(properties,"copy_kind")->valueint; 
                    stmt = new CopyStmt(srcNode, dstNode, edgeFlag, edge_id, value, copy_kind, icfgNode );
                    CopyStmt* copyStmt = SVFUtil::cast<CopyStmt>(stmt);
                    if (!pag->hasEdge(copyStmt, SVFStmt::Copy))
                    {
                        pag->addCopyStmt(copyStmt);
                    }
                }
                else if (edgeType == "StoreStmt")
                {
                    stmt = new StoreStmt(srcNode, dstNode, edgeFlag, edge_id, value, icfgNode);
                    StoreStmt* storeStmt = SVFUtil::cast<StoreStmt>(stmt);
                    if (!pag->hasEdge(storeStmt, SVFStmt::Store))
                    {
                        pag->addStoreStmt(storeStmt, srcNode, dstNode);
                    }
                }
                else if (edgeType == "LoadStmt")
                {
                    stmt = new LoadStmt(srcNode, dstNode, edgeFlag, edge_id, value, icfgNode);
                    LoadStmt* loadStmt = SVFUtil::cast<LoadStmt>(stmt);
                    if (!pag->hasEdge(loadStmt, SVFStmt::Load))
                    {
                        pag->addLoadStmt(loadStmt);
                    }
                }
                else if (edgeType == "GepStmt")
                {
                    s64_t fldIdx = cJSON_GetObjectItem(properties, "ap_fld_idx")->valueint;
                    if (fldIdx == -1)
                    {
                        fldIdx = 0;
                    }
                    bool variant_field = cJSON_IsTrue(cJSON_GetObjectItem(properties,"variant_field"));
                    int ap_gep_pointee_type_id = cJSON_GetObjectItem(properties, "ap_gep_pointee_type_id")->valueint;
                    const SVFType* gepPointeeType = nullptr;
                    if (ap_gep_pointee_type_id != -1)
                    {
                        gepPointeeType = pag->getSVFType(ap_gep_pointee_type_id);
                    }
                    AccessPath* ap = nullptr;
                    if (nullptr != gepPointeeType)
                    {
                        ap = new AccessPath(fldIdx, gepPointeeType);
                    }
                    else
                    {
                        ap = new AccessPath(fldIdx);
                        if (ap_gep_pointee_type_id != -1)
                            SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching SVFType found for ap_gep_pointee_type_id: " << ap_gep_pointee_type_id << " when updating GepStmt:"<<edge_id<< "\n";
                    }
                    cJSON* ap_idx_operand_pairs_node = cJSON_GetObjectItem(properties, "ap_idx_operand_pairs");
                    std::string ap_idx_operand_pairs = "";
                    if (nullptr != ap_idx_operand_pairs_node && nullptr != ap_idx_operand_pairs_node->valuestring)
                    {
                        ap_idx_operand_pairs = ap_idx_operand_pairs_node->valuestring;   
                    }
                    parseAPIdxOperandPairsString(ap_idx_operand_pairs, pag, ap);
                    
                    stmt = new GepStmt(srcNode, dstNode, edgeFlag, edge_id, value, icfgNode, *ap, variant_field);
                    GepStmt* gepStmt = SVFUtil::cast<GepStmt>(stmt);
                    if (!pag->hasEdge(gepStmt, SVFStmt::Gep))
                    {
                        pag->addGepStmt(gepStmt);
                    }
                }
                else if (edgeType == "CallPE")
                {
                    int call_icfg_node_id = cJSON_GetObjectItem(properties,"call_icfg_node_id")->valueint;
                    int fun_entry_icfg_node_id = cJSON_GetObjectItem(properties,"fun_entry_icfg_node_id")->valueint;
                    const CallICFGNode* callICFGNode = nullptr;
                    const FunEntryICFGNode* funEntryICFGNode = nullptr;
                    if (call_icfg_node_id != -1)
                    {
                        callICFGNode = SVFUtil::dyn_cast<CallICFGNode>(pag->getICFG()->getGNode(call_icfg_node_id));
                        if (nullptr == callICFGNode)
                        {
                            SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching CallICFGNode found for id: " << call_icfg_node_id << "\n";
                            continue;
                        }
                    }
                    if (fun_entry_icfg_node_id != -1)
                    {
                        funEntryICFGNode = SVFUtil::dyn_cast<FunEntryICFGNode>(pag->getICFG()->getGNode(fun_entry_icfg_node_id));
                        if (nullptr == funEntryICFGNode)
                        {
                            SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching FunEntryICFGNode found for id: " << fun_entry_icfg_node_id << "\n";
                            continue;
                        }
                    }
                    stmt = new CallPE(srcNode, dstNode, edgeFlag, edge_id, value, icfgNode, callICFGNode, funEntryICFGNode);
                    CallPE* callPE = SVFUtil::cast<CallPE>(stmt);
                    if (!pag->hasEdge(callPE, SVFStmt::Call))
                    {
                        pag->addCallPE(callPE, srcNode, dstNode);
                    }
                    id2CallPEMap[edge_id] = callPE;
                }
                else if (edgeType == "TDForkPE")
                {
                    int call_icfg_node_id = cJSON_GetObjectItem(properties,"call_icfg_node_id")->valueint;
                    int fun_entry_icfg_node_id = cJSON_GetObjectItem(properties,"fun_entry_icfg_node_id")->valueint;
                    const CallICFGNode* callICFGNode = nullptr;
                    const FunEntryICFGNode* funEntryICFGNode = nullptr;
                    if (call_icfg_node_id != -1)
                    {
                        callICFGNode = SVFUtil::dyn_cast<CallICFGNode>(pag->getICFG()->getGNode(call_icfg_node_id));
                        if (nullptr == callICFGNode)
                        {
                            SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching CallICFGNode found for id: " << call_icfg_node_id << "\n";
                            continue;
                        }
                    }
                    if (fun_entry_icfg_node_id != -1)
                    {
                        funEntryICFGNode = SVFUtil::dyn_cast<FunEntryICFGNode>(pag->getICFG()->getGNode(fun_entry_icfg_node_id));
                        if (nullptr == funEntryICFGNode)
                        {
                            SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching FunEntryICFGNode found for id: " << fun_entry_icfg_node_id << "\n";
                            continue;
                        }
                    }
                    stmt = new TDForkPE(srcNode, dstNode, edgeFlag, edge_id, value, icfgNode, callICFGNode, funEntryICFGNode);
                    TDForkPE* forkPE = SVFUtil::cast<TDForkPE>(stmt);
                    if (!pag->hasEdge(forkPE, SVFStmt::ThreadFork))
                    {
                        pag->addCallPE(forkPE, srcNode, dstNode);
                    }
                    id2CallPEMap[edge_id] = forkPE;
                }
                else if (edgeType == "RetPE")
                {
                    int call_icfg_node_id = cJSON_GetObjectItem(properties,"call_icfg_node_id")->valueint;
                    int fun_exit_icfg_node_id = cJSON_GetObjectItem(properties,"fun_exit_icfg_node_id")->valueint;
                    const CallICFGNode* callICFGNode = nullptr;
                    const FunExitICFGNode* funExitICFGNode = nullptr;
                    if (call_icfg_node_id != -1)
                    {
                        callICFGNode = SVFUtil::dyn_cast<CallICFGNode>(pag->getICFG()->getGNode(call_icfg_node_id));
                        if (nullptr == callICFGNode)
                        {
                            SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching CallICFGNode found for id: " << call_icfg_node_id << "\n";
                            continue;  
                        }
                    }
                    if (fun_exit_icfg_node_id != -1)
                    {
                        funExitICFGNode = SVFUtil::dyn_cast<FunExitICFGNode>(pag->getICFG()->getGNode(fun_exit_icfg_node_id));
                        if (nullptr == funExitICFGNode)
                        {
                            SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching FunExitICFGNode found for id: " << fun_exit_icfg_node_id << "\n";
                            continue;
                        }
                    }
                    stmt = new RetPE(srcNode, dstNode, edgeFlag, edge_id, value, icfgNode,callICFGNode, funExitICFGNode);
                    RetPE* retPE = SVFUtil::cast<RetPE>(stmt);
                    if (!pag->hasEdge(retPE, SVFStmt::Ret))
                    {
                        pag->addRetPE(retPE, srcNode, dstNode);
                    }
                    id2RetPEMap[edge_id] = retPE;
                }
                else if (edgeType == "RetPETDJoinPE")
                {
                    int call_icfg_node_id = cJSON_GetObjectItem(properties,"call_icfg_node_id")->valueint;
                    int fun_exit_icfg_node_id = cJSON_GetObjectItem(properties,"fun_exit_icfg_node_id")->valueint;
                    const CallICFGNode* callICFGNode = nullptr;
                    const FunExitICFGNode* funExitICFGNode = nullptr;
                    if (call_icfg_node_id != -1)
                    {
                        callICFGNode = SVFUtil::dyn_cast<CallICFGNode>(pag->getICFG()->getGNode(call_icfg_node_id));
                        if (nullptr == callICFGNode)
                        {
                            SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching CallICFGNode found for id: " << call_icfg_node_id << "\n";
                            continue;
                        }
                    }
                    if (fun_exit_icfg_node_id != -1)
                    {
                        funExitICFGNode = SVFUtil::dyn_cast<FunExitICFGNode>(pag->getICFG()->getGNode(fun_exit_icfg_node_id));
                        if (nullptr == funExitICFGNode)
                        {
                            SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching FunExitICFGNode found for id: " << fun_exit_icfg_node_id << "\n";
                            continue;
                        }
                    }
                    stmt = new TDJoinPE(srcNode, dstNode, edgeFlag, edge_id, value, icfgNode, callICFGNode, funExitICFGNode);
                    TDJoinPE* joinPE = SVFUtil::cast<TDJoinPE>(stmt);
                    if (!pag->hasEdge(joinPE, SVFStmt::ThreadJoin))
                    {
                        pag->addRetPE(joinPE, srcNode, dstNode);
                    }
                    id2RetPEMap[edge_id] = joinPE;
                }
                else if (edgeType == "PhiStmt")
                {
                    std::vector<SVFVar*> opVarNodes;
                    std::string op_var_node_ids = cJSON_GetObjectItem(properties, "op_var_node_ids")->valuestring;
                    parseOpVarString(op_var_node_ids, pag, opVarNodes);
                    stmt = new PhiStmt(srcNode, dstNode, edgeFlag, edge_id, value, icfgNode, opVarNodes);
                    PhiStmt* phiStmt = SVFUtil::cast<PhiStmt>(stmt);
                    std::string op_icfg_nodes_ids = cJSON_GetObjectItem(properties, "op_icfg_nodes_ids")->valuestring;
                    if (!op_icfg_nodes_ids.empty())
                    {
                        std::vector<int> opICFGNodeIds = parseElements2Container<std::vector<int>>(op_icfg_nodes_ids);
                        std::vector<const ICFGNode*> opICFGNodes;
                        for (int icfgNodeId : opICFGNodeIds)
                        {
                            ICFGNode* opICFGNode = pag->getICFG()->getGNode(icfgNodeId);
                            if (nullptr != opICFGNode)
                            {
                                opICFGNodes.push_back(opICFGNode);
                            }
                            else
                            {
                                SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching ICFGNode found for id: " << icfgNodeId << "\n";
                            }
                        }
                        phiStmt->setOpICFGNodeVec(opICFGNodes);
                    }  
                    pag->addPhiStmt(phiStmt, srcNode, dstNode);
                }
                else if (edgeType == "SelectStmt")
                {
                    std::vector<SVFVar*> opVarNodes;
                    std::string op_var_node_ids = cJSON_GetObjectItem(properties, "op_var_node_ids")->valuestring;
                    parseOpVarString(op_var_node_ids, pag, opVarNodes);
                    int condition_svf_var_node_id = cJSON_GetObjectItem(properties, "condition_svf_var_node_id")->valueint;
                    SVFVar* condition = pag->getGNode(condition_svf_var_node_id);
                    stmt = new SelectStmt(srcNode, dstNode, edgeFlag, edge_id, condition, value, icfgNode, opVarNodes);
                    SelectStmt* selectStmt = SVFUtil::cast<SelectStmt>(stmt);
                    if (!pag->hasEdge(selectStmt, SVFStmt::Select))
                    {
                        pag->addSelectStmt(selectStmt, srcNode, dstNode);
                    }
                }
                else if (edgeType == "CmpStmt")
                {
                    std::vector<SVFVar*> opVarNodes;
                    std::string op_var_node_ids = cJSON_GetObjectItem(properties, "op_var_node_ids")->valuestring;
                    parseOpVarString(op_var_node_ids, pag, opVarNodes);
                    u32_t predicate = cJSON_GetObjectItem(properties, "predicate")->valueint;
                    stmt = new CmpStmt(srcNode, dstNode, edgeFlag, edge_id, value, predicate, icfgNode, opVarNodes);
                    CmpStmt* cmpStmt = SVFUtil::cast<CmpStmt>(stmt);
                    if (!pag->hasEdge(cmpStmt, SVFStmt::Cmp))
                    {
                        pag->addCmpStmt(cmpStmt, srcNode, dstNode);
                    }
                    
                }
                else if (edgeType == "BinaryOPStmt")
                {
                    std::vector<SVFVar*> opVarNodes;
                    std::string op_var_node_ids = cJSON_GetObjectItem(properties, "op_var_node_ids")->valuestring;
                    parseOpVarString(op_var_node_ids, pag, opVarNodes);
                    u32_t op_code = cJSON_GetObjectItem(properties, "op_code")->valueint;
                    stmt = new BinaryOPStmt(srcNode, dstNode, edgeFlag, edge_id, value, op_code, icfgNode, opVarNodes);
                    BinaryOPStmt* binaryOpStmt = SVFUtil::cast<BinaryOPStmt>(stmt);
                    if (!pag->hasEdge(binaryOpStmt, SVFStmt::BinaryOp))
                    {
                        pag->addBinaryOPStmt(binaryOpStmt, srcNode, dstNode);
                    }
                }
                else if (edgeType == "UnaryOPStmt")
                {
                    u32_t op_code = cJSON_GetObjectItem(properties, "op_code")->valueint;
                    stmt = new UnaryOPStmt(srcNode, dstNode, edgeFlag, edge_id, value, op_code, icfgNode);
                    UnaryOPStmt* unaryOpStmt = SVFUtil::cast<UnaryOPStmt>(stmt);
                    if (!pag->hasEdge(unaryOpStmt, SVFStmt::UnaryOp))
                    {
                        pag->addUnaryOPStmt(unaryOpStmt, srcNode, dstNode);
                    }
                }
                else if (edgeType == "BranchStmt")
                {
                    int condition_svf_var_node_id = cJSON_GetObjectItem(properties, "condition_svf_var_node_id")->valueint;
                    int br_inst_svf_var_node_id = cJSON_GetObjectItem(properties, "br_inst_svf_var_node_id")->valueint;
                    const SVFVar* condition = pag->getGNode(condition_svf_var_node_id);
                    const SVFVar* brInst = pag->getGNode(br_inst_svf_var_node_id);
                    if (condition == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching condition SVFVar found for id: " << condition_svf_var_node_id << "\n";
                        continue;
                    }
                    if (brInst == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching brInst SVFVar found for id: " << br_inst_svf_var_node_id << "\n";
                        continue;
                    }
                    std::string successorsStr = cJSON_GetObjectItem(properties, "successors")->valuestring;
                    std::vector<std::pair<int, s32_t>> successorsIdVec = parseSuccessorsPairSetFromString(successorsStr);
                    std::vector<std::pair<const ICFGNode*, s32_t>> successors;
                    for (auto& pair : successorsIdVec)
                    {
                        const ICFGNode* succ = pag->getICFG()->getGNode(pair.first);
                        if (nullptr != succ)
                        {
                            successors.push_back(std::make_pair(succ, pair.second));
                        }
                        else 
                        {
                            SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching ICFGNode found for id: " << pair.first << "\n";
                        }
                    }
                    stmt = new BranchStmt(srcNode, dstNode, edgeFlag, edge_id, value, successors, condition, brInst, icfgNode);
                    BranchStmt* branchStmt = SVFUtil::cast<BranchStmt>(stmt);
                    if (!pag->hasEdge(branchStmt, SVFStmt::Branch))
                    {
                        pag->addBranchStmt(branchStmt, srcNode, dstNode);
                    }
                }
                stmt->setBasicBlock(bb);
                stmt->setCallEdgeLabelCounter(static_cast<u64_t>(call_edge_label_counter));
                stmt->setStoreEdgeLabelCounter(static_cast<u64_t>(store_edge_label_counter));
                stmt->setMultiOpndLabelCounter(static_cast<u64_t>(multi_opnd_label_counter));
                std::string inst2_label_map;
                cJSON* inst2_label_map_item = cJSON_GetObjectItem(properties,"inst2_label_map");
                if (nullptr != inst2_label_map_item->valuestring)
                {
                    inst2_label_map = inst2_label_map_item->valuestring;
                } 
                else {
                    inst2_label_map = "";
                }
                std::string var2_label_map;
                cJSON* var2_label_map_item = cJSON_GetObjectItem(properties,"var2_label_map");
                if (nullptr != var2_label_map_item->valuestring)
                {
                    var2_label_map = var2_label_map_item->valuestring;
                } 
                else {
                    var2_label_map = "";
                }
                Map<int, u32_t> inst2_label_map_ids = parseLabelMapFromString<Map<int, u32_t>>(inst2_label_map);
                Map<int, u32_t> var2_label_map_ids = parseLabelMapFromString<Map<int, u32_t>>(var2_label_map);
                if (!inst2_label_map_ids.empty())
                {
                    for (auto& [id, label] : inst2_label_map_ids)
                    {
                        const ICFGNode* icfgNode = nullptr;
                        if (id != -1)
                        {
                            icfgNode = pag->getICFG()->getGNode(id);
                            if (nullptr == icfgNode)
                            {
                                SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching ICFGNode found for id: " << id << " when parsing inst2_label_map_ids\n";
                            }
                        }
                        stmt->addInst2Labeled(icfgNode, label);
                    }
                }
                if (!var2_label_map_ids.empty())
                {
                    for (auto& [id, label] : var2_label_map_ids)
                    {
                        const SVFVar* var = nullptr;
                        if (id != -1)
                        {
                            var = pag->getGNode(id);
                            if (nullptr == var)
                            {
                                SVFUtil::outs() << "Warning: [readPAGEdgesFromDB] No matching SVFVar found for id: " << id << " when parsing var2_label_map_ids\n";
                            }
                        }
                        stmt->addVar2Labeled(var, label);
                    }
                }
                skip += 1;
                edgeId2SVFStmtMap[stmt->getEdgeID()] = stmt;
            }
            cJSON_Delete(root);
        }
    }
}

void GraphDBClient::parseOpVarString(std::string& op_var_node_ids, SVFIR* pag, std::vector<SVFVar*>& opVarNodes)
{
    if (!op_var_node_ids.empty())
    {
        std::vector<int> opVarNodeIds = parseElements2Container<std::vector<int>>(op_var_node_ids);
        for (int varId : opVarNodeIds)
        {
            SVFVar* var = pag->getGNode(varId);
            if (nullptr != var)
            {
                opVarNodes.push_back(var);
            }
            else
            {
                SVFUtil::outs() << "Warning: [parseOpVarString] No matching SVFVar found for id: " << varId << "\n";
            }
        }
    }
}

void GraphDBClient::initialSVFPAGNodesFromDB(lgraph::RpcClient* connection, const std::string& dbname, SVFIR* pag)
{
    SVFUtil::outs()<< "Initial SVF PAG nodes from DB....\n";
    readPAGNodesFromDB(connection, dbname, "ValVar", pag);
    readPAGNodesFromDB(connection, dbname, "ObjVar", pag);
    readPAGNodesFromDB(connection, dbname, "ArgValVar", pag);
    readPAGNodesFromDB(connection, dbname, "GepValVar", pag);
    readPAGNodesFromDB(connection, dbname, "BaseObjVar", pag);
    readPAGNodesFromDB(connection, dbname, "GepObjVar", pag);
    readPAGNodesFromDB(connection, dbname, "HeapObjVar", pag);
    readPAGNodesFromDB(connection, dbname, "StackObjVar", pag);
    readPAGNodesFromDB(connection, dbname, "FunObjVar", pag);
    readPAGNodesFromDB(connection, dbname, "FunValVar", pag);
    readPAGNodesFromDB(connection, dbname, "GlobalValVar", pag);
    readPAGNodesFromDB(connection, dbname, "ConstAggValVar", pag);
    readPAGNodesFromDB(connection, dbname, "ConstDataValVar", pag);
    readPAGNodesFromDB(connection, dbname, "BlackHoleValVar", pag);
    readPAGNodesFromDB(connection, dbname, "ConstFPValVar", pag);
    readPAGNodesFromDB(connection, dbname, "ConstIntValVar", pag);
    readPAGNodesFromDB(connection, dbname, "ConstNullPtrValVar", pag);
    readPAGNodesFromDB(connection, dbname, "GlobalObjVar", pag);
    readPAGNodesFromDB(connection, dbname, "ConstAggObjVar", pag);
    readPAGNodesFromDB(connection, dbname, "ConstDataObjVar", pag);
    readPAGNodesFromDB(connection, dbname, "ConstFPObjVar", pag);
    readPAGNodesFromDB(connection, dbname, "ConstIntObjVar", pag);
    readPAGNodesFromDB(connection, dbname, "ConstNullPtrObjVar", pag);
    readPAGNodesFromDB(connection, dbname, "RetValPN", pag);
    readPAGNodesFromDB(connection, dbname, "VarArgValPN", pag);
    readPAGNodesFromDB(connection, dbname, "DummyValVar", pag);
    readPAGNodesFromDB(connection, dbname, "DummyObjVar", pag);
}

void GraphDBClient::updatePAGNodesFromDB(lgraph::RpcClient* connection, const std::string& dbname, SVFIR* pag)
{
    SVFUtil::outs()<< "Updating SVF PAG nodes from DB....\n";
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "ValVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "ObjVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "ArgValVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "GepValVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "BaseObjVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "GepObjVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "HeapObjVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "StackObjVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "FunObjVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "FunValVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "GlobalValVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "ConstAggValVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "ConstDataValVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "BlackHoleValVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "ConstFPValVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "ConstIntValVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "ConstNullPtrValVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "GlobalObjVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "ConstAggObjVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "ConstDataObjVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "ConstFPObjVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "ConstIntObjVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "ConstNullPtrObjVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "RetValPN", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "VarArgValPN", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "DummyValVar", pag);
    updateSVFPAGNodesAttributesFromDB(connection, dbname, "DummyObjVar", pag);
}

void GraphDBClient::updateSVFValVarAtrributes(cJSON* properties, ValVar* var, SVFIR* pag)
{
    int icfg_node_id = cJSON_GetObjectItem(properties, "icfg_node_id")->valueint;
    if (icfg_node_id != -1)
    {
        ICFGNode* icfgNode = pag->getICFG()->getGNode(icfg_node_id);
        if (nullptr != icfgNode)
        {
            var->updateSVFValVarFromDB(icfgNode);
        }
        else
        {
            SVFUtil::outs() << "Warning: [updateSVFValVarAtrributes] No matching ICFGNode found for id: " << icfg_node_id << " when update SVFVar:"<<var->getId()<<"\n";
        }
    }
}

void GraphDBClient::updateSVFBaseObjVarAtrributes(cJSON* properties, BaseObjVar* var, SVFIR* pag)
{
    int icfg_node_id = cJSON_GetObjectItem(properties, "icfg_node_id")->valueint;
    if (icfg_node_id != -1)
    {
        ICFGNode* icfgNode = pag->getICFG()->getGNode(icfg_node_id);
        if (nullptr != icfgNode)
        {
            var->setICFGNode(icfgNode);
        }
        else
        {
            SVFUtil::outs() << "Warning: [updateSVFValVarAtrributes] No matching ICFGNode found for id: " << icfg_node_id << " when update SVFVar:"<<var->getId()<<"\n";
        }
    }
}

void GraphDBClient::updateFunObjVarAttributes(cJSON* properties, FunObjVar* var, SVFIR* pag)
{
    int real_def_fun_node_id = cJSON_GetObjectItem(properties, "real_def_fun_node_id")->valueint;
    const FunObjVar* realDefFunNode = id2funObjVarsMap[real_def_fun_node_id];
    if (nullptr != realDefFunNode)
    {
        var->setRelDefFun(realDefFunNode);
    }
    else
    {
        SVFUtil::outs() << "Warning: [updateFunObjVarAttributes] No matching FunObjVar found for id: " << real_def_fun_node_id <<" when updating FunObjVar:"<<var->getId()<< "\n";
    }
    
    int exit_bb_id = cJSON_GetObjectItem(properties, "exit_bb_id")->valueint;
    if (exit_bb_id != -1)
    {
        SVFBasicBlock* exitBB = var->getBasicBlockGraph()->getGNode(exit_bb_id);
        if (nullptr != exitBB)
        {
            var->updateExitBlock(exitBB);
        }
        else
        {
            SVFUtil::outs() << "Warning: [updateFunObjVarAttributes] No matching BasicBlock found for id: " << exit_bb_id <<" when updating FunObjVar:"<<var->getId()<< "\n";
        }
    }

    SVFLoopAndDomInfo* loopAndDom = new SVFLoopAndDomInfo();
    var->setLoopAndDomInfo(loopAndDom);

    std::string reachable_bbs = cJSON_GetObjectItem(properties, "reachable_bbs")->valuestring;
    std::string dt_bbs_map = cJSON_GetObjectItem(properties, "dt_bbs_map")->valuestring;
    std::string pdt_bbs_map = cJSON_GetObjectItem(properties, "pdt_bbs_map")->valuestring;
    std::string df_bbs_map = cJSON_GetObjectItem(properties, "df_bbs_map")->valuestring;
    std::string bb2_loop_map = cJSON_GetObjectItem(properties, "bb2_loop_map")->valuestring;
    std::string bb2_p_dom_level = cJSON_GetObjectItem(properties, "bb2_p_dom_level")->valuestring;
    std::string bb2_pi_dom = cJSON_GetObjectItem(properties, "bb2_pi_dom")->valuestring;

    if (!reachable_bbs.empty())
    {
        std::vector<int> BBListVec = parseElements2Container<std::vector<int>>(reachable_bbs);
        std::vector<const SVFBasicBlock*> reachableBBs;
        for (auto& bbId : BBListVec)
        {
            SVFBasicBlock* bb = var->getBasicBlockGraph()->getGNode(bbId);
            if (nullptr != bb)
            {
                reachableBBs.push_back(bb);
            }
            else
            {
                SVFUtil::outs() << "Warning: [updateFunObjVarAttributes] No matching BasicBlock found for id: " << bbId <<" when updating FunObjVar:"<<var->getId()<< "\n";
            }
        }
        loopAndDom->setReachableBBs(reachableBBs);
    }

    if (!dt_bbs_map.empty())
    {
        Map<const SVFBasicBlock*, Set<const SVFBasicBlock*>> dtBBsMap;

        Map<int, Set<int>> dt_bbs_map_ids = parseBBsMapFromString<Map<int, Set<int>>>(dt_bbs_map);
        for (auto& [bbId, bbSetIds] : dt_bbs_map_ids)
        {
            SVFBasicBlock* bb = var->getBasicBlockGraph()->getGNode(bbId);
            if (nullptr != bb)
            {
                Set<const SVFBasicBlock*> dtBBSet;
                for (auto& bbSetId : bbSetIds)
                {
                    SVFBasicBlock* bbSet = var->getBasicBlockGraph()->getGNode(bbSetId);
                    if (nullptr != bbSet)
                    {
                        dtBBSet.insert(bbSet);
                    }
                    else
                    {
                        SVFUtil::outs() << "Warning: [updateFunObjVarAttributes] No matching BasicBlock found for id: " << bbSetId <<" when updating FunObjVar:"<<var->getId()<< "\n";
                    }
                }
                dtBBsMap[bb] = dtBBSet;
            }
            else
            {
                SVFUtil::outs() << "Warning: [updateFunObjVarAttributes] No matching BasicBlock found for id: " << bbId <<" when updating FunObjVar:"<<var->getId()<< "\n";
            }
        }
        loopAndDom->setDomTreeMap(dtBBsMap);
    }

    if (!pdt_bbs_map.empty())
    {
        Map<const SVFBasicBlock*, Set<const SVFBasicBlock*>> pdtBBsMap;

        Map<int, Set<int>> pdt_bbs_map_ids = parseBBsMapFromString<Map<int, Set<int>>>(pdt_bbs_map);
        for (auto& [bbId, bbSetIds] : pdt_bbs_map_ids)
        {
            SVFBasicBlock* bb = var->getBasicBlockGraph()->getGNode(bbId);
            if (nullptr != bb)
            {
                Set<const SVFBasicBlock*> pdtBBSet;
                for (auto& bbSetId : bbSetIds)
                {
                    SVFBasicBlock* bbSet = var->getBasicBlockGraph()->getGNode(bbSetId);
                    if (nullptr != bbSet)
                    {
                        pdtBBSet.insert(bbSet);
                    }
                    else
                    {
                        SVFUtil::outs() << "Warning: [updateFunObjVarAttributes] No matching BasicBlock found for id: " << bbSetId <<" when updating FunObjVar:"<<var->getId()<< "\n";
                    }
                }
                pdtBBsMap[bb] = pdtBBSet;
            }
            else
            {
                SVFUtil::outs() << "Warning: [updateFunObjVarAttributes] No matching BasicBlock found for id: " << bbId <<" when updating FunObjVar:"<<var->getId()<< "\n";
            }
        }
        loopAndDom->setPostDomTreeMap(pdtBBsMap);
    }

    if (!df_bbs_map.empty())
    {
        Map<const SVFBasicBlock*, Set<const SVFBasicBlock*>> dfBBsMap;

        Map<int, Set<int>> df_bbs_map_ids = parseBBsMapFromString<Map<int, Set<int>>>(df_bbs_map);
        for (auto& [bbId, bbSetIds] : df_bbs_map_ids)
        {
            SVFBasicBlock* bb = var->getBasicBlockGraph()->getGNode(bbId);
            if (nullptr != bb)
            {
                Set<const SVFBasicBlock*> dfBBSet;
                for (auto& bbSetId : bbSetIds)
                {
                    SVFBasicBlock* bbSet = var->getBasicBlockGraph()->getGNode(bbSetId);
                    if (nullptr != bbSet)
                    {
                        dfBBSet.insert(bbSet);
                    }
                    else
                    {
                        SVFUtil::outs() << "Warning: [updateFunObjVarAttributes] No matching BasicBlock found for id: " << bbSetId <<" when updating FunObjVar:"<<var->getId()<< "\n";
                    }
                }
                dfBBsMap[bb] = dfBBSet;
            }
            else
            {
                SVFUtil::outs() << "Warning: [updateFunObjVarAttributes] No matching BasicBlock found for id: " << bbId <<" when updating FunObjVar:"<<var->getId()<< "\n";
            }
        }
        loopAndDom->setDomFrontierMap(dfBBsMap);
    }

    if (!bb2_loop_map.empty())
    {
        Map<const SVFBasicBlock*, std::vector<const SVFBasicBlock*>> bb2LoopMap;

        Map<int, std::vector<int>> bb2_loop_map_ids = parseBBsMapFromString<Map<int, std::vector<int>>>(bb2_loop_map);
        for (auto& [bbId, bbSetIds] : bb2_loop_map_ids)
        {
            SVFBasicBlock* bb = var->getBasicBlockGraph()->getGNode(bbId);
            if (nullptr != bb)
            {
                std::vector<const SVFBasicBlock*> loopBBs;
                for (auto& bbSetId : bbSetIds)
                {
                    SVFBasicBlock* bbSet = var->getBasicBlockGraph()->getGNode(bbSetId);
                    if (nullptr != bbSet)
                    {
                        loopBBs.push_back(bbSet);
                    }
                    else
                    {
                        SVFUtil::outs() << "Warning: [updateFunObjVarAttributes] No matching BasicBlock found for id: " << bbSetId <<" when updating FunObjVar:"<<var->getId()<< "\n";
                    }
                }
                bb2LoopMap[bb] = loopBBs;
            }
            else
            {
                SVFUtil::outs() << "Warning: [updateFunObjVarAttributes] No matching BasicBlock found for id: " << bbId <<" when updating FunObjVar:"<<var->getId()<< "\n";
            }
        }
        loopAndDom->setBB2LoopMap(bb2LoopMap);
    }
    
    if (!bb2_p_dom_level.empty())
    {
        Map<const SVFBasicBlock*, u32_t> bb2PdomLevel;
        Map<int, u32_t> bb2_p_dom_level_ids = parseLabelMapFromString<Map<int, u32_t>>(bb2_p_dom_level);
        for (auto& [bbId, value] : bb2_p_dom_level_ids)
        {
            SVFBasicBlock* bb = nullptr;
            if (bbId != -1)
            {
                bb = var->getBasicBlockGraph()->getGNode(bbId);
            }
            if (nullptr != bb)
            {
                bb2PdomLevel[bb] = value;
            }
            else
            {
                SVFUtil::outs() << "Warning: [updateFunObjVarAttributes] No matching BasicBlock found for id: " << bbId <<" when updating FunObjVar:"<<var->getId()<< "\n";
            }
        }

        loopAndDom->setBB2PdomLevel(bb2PdomLevel);
    }

    if (!bb2_pi_dom.empty())
    {
        Map<const SVFBasicBlock*, const SVFBasicBlock*> bb2PiDom;
        
        Map<int , int> bb2_pi_dom_ids = parseBB2PiMapFromString<Map<int, int>>(bb2_pi_dom);
        for (auto& [key,value] : bb2_pi_dom_ids)
        {
            SVFBasicBlock* keyBB = nullptr;
            SVFBasicBlock* valueBB = nullptr;
            if (key != -1)
            {
                keyBB = var->getBasicBlockGraph()->getGNode(key);
            }
            if (value != -1)
            {
                valueBB = var->getBasicBlockGraph()->getGNode(value);
            }
            bb2PiDom[keyBB] = valueBB;
        }
        loopAndDom->setBB2PIdom(bb2PiDom);
    }
}
void GraphDBClient::updateGepValVarAttributes(cJSON* properties, GepValVar* var, SVFIR* pag)
{
    int base_val_id = cJSON_GetObjectItem(properties, "base_val_id")->valueint;
    ValVar* baseVal = SVFUtil::dyn_cast<ValVar>(pag->getGNode(base_val_id));
    if (nullptr != baseVal)
    {
        var->setBaseNode(baseVal);
    }
    else
    {
        SVFUtil::outs() << "Warning: [updateGepValVarAttributes] No matching ValVar found for id: "
                        << base_val_id << " when updating GepValVar:" << var->getId()
                        << "\n";
    }
    s64_t fldIdx = cJSON_GetObjectItem(properties, "ap_fld_idx")->valueint;
    int ap_gep_pointee_type_id = cJSON_GetObjectItem(properties, "ap_gep_pointee_type_id")->valueint;
    const SVFType* gepPointeeType = nullptr;
    if (ap_gep_pointee_type_id != -1)
    {
        gepPointeeType = pag->getSVFType(ap_gep_pointee_type_id);
    }
    AccessPath* ap = nullptr;
    if (nullptr != gepPointeeType)
    {
        ap = new AccessPath(fldIdx, gepPointeeType);
    }
    else
    {
        ap = new AccessPath(fldIdx);
        if (ap_gep_pointee_type_id != -1)
            SVFUtil::outs() << "Warning: [updateGepValVarAttributes] No matching SVFType found for ap_gep_pointee_type_id: " << ap_gep_pointee_type_id << " when updating GepValVar:"<<var->getId()<< "\n";
    }

    cJSON* ap_idx_operand_pairs_node = cJSON_GetObjectItem(properties, "ap_idx_operand_pairs");
    std::string ap_idx_operand_pairs = "";
    if (nullptr != ap_idx_operand_pairs_node && nullptr != ap_idx_operand_pairs_node->valuestring)
    {
        ap_idx_operand_pairs = ap_idx_operand_pairs_node->valuestring;   
    }
    parseAPIdxOperandPairsString(ap_idx_operand_pairs, pag, ap);
    var->setAccessPath(ap);
    int llvm_var_inst_id = cJSON_GetObjectItem(properties, "llvm_var_inst_id")->valueint;
    pag->addGepValObjFromDB(llvm_var_inst_id, var);
}

void GraphDBClient::parseAPIdxOperandPairsString(const std::string& ap_idx_operand_pairs, SVFIR* pag, AccessPath* ap)
{
    if (!ap_idx_operand_pairs.empty())
    {
        std::vector<std::pair<int, std::string>> pairVec = parseIdxOperandPairsString(ap_idx_operand_pairs);
        for(auto& pair : pairVec)
        {
            int varId = pair.first;
            std::string typeIdStr = pair.second;
            const SVFType* type;
            if (typeIdStr != "NULL")
            {
                int typeId = std::stoi(typeIdStr);
                type = pag->getSVFType(typeId);
                if (nullptr == type)
                {
                    SVFUtil::outs() << "Warning: [parseAPIdxOperandPairsString] No matching SVFType found for type: " << typeIdStr << " when parsing IdxOperandPair\n";
                }
            }
            else 
            {
                type = nullptr;
            }
            const SVFVar* var = pag->getGNode(varId);
            if (nullptr != var)
            {
                std::pair<const SVFVar*, const SVFType*> pair = std::make_pair(var, type);
                ap->addIdxOperandPair(pair);
            }
            else
            {
                SVFUtil::outs() << "Warning: [parseAPIdxOperandPairsString] No matching ValVar found for id: " << varId <<" when parsing IdxOperandPair \n";
            }
        }
    }
}

void GraphDBClient::updateSVFPAGNodesAttributesFromDB(lgraph::RpcClient* connection, const std::string& dbname, std::string nodeType, SVFIR* pag)
{
    int skip = 0;
    int limit = 1000;
    while (true)
    {
        std::string result;
        std::string queryStatement = " MATCH (node:"+nodeType+") RETURN node SKIP "+ std::to_string(skip)+" LIMIT "+std::to_string(limit);
        cJSON* root = queryFromDB(connection, dbname, queryStatement);
        if (nullptr == root)
        {
            break;
        }
        else
        {
            cJSON* node;
            cJSON_ArrayForEach(node, root)
            {
                cJSON* data = cJSON_GetObjectItem(node, "node");
                if (!data)
                    continue;
                cJSON* properties = cJSON_GetObjectItem(data, "properties");
                if (!properties)
                    continue;
                int id = cJSON_GetObjectItem(properties,"id")->valueint;
                if (nodeType == "ConstNullPtrValVar")
                {
                    ConstNullPtrValVar* var = SVFUtil::dyn_cast<ConstNullPtrValVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching ConstNullPtrValVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFValVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "ConstIntValVar")
                {
                    ConstIntValVar* var = SVFUtil::dyn_cast<ConstIntValVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching ConstIntValVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFValVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "ConstFPValVar")
                {
                    ConstFPValVar* var = SVFUtil::dyn_cast<ConstFPValVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching ConstFPValVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFValVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "ArgValVar")
                {
                    ArgValVar* var = SVFUtil::dyn_cast<ArgValVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching ArgValVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFValVarAtrributes(properties, var, pag);
                    int cg_node_id = cJSON_GetObjectItem(properties, "cg_node_id")->valueint;
                    FunObjVar* cgNode = id2funObjVarsMap[cg_node_id];
                    if (nullptr != cgNode)
                    {
                        var->addCGNodeFromDB(cgNode);
                    }
                    else
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching FunObjVar found for id: " << cg_node_id <<" when updating ArgValVar:"<< id << "\n";
                    }
                }
                else if (nodeType == "BlackHoleValVar")
                {
                    BlackHoleValVar* var = SVFUtil::dyn_cast<BlackHoleValVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching BlackHoleValVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFValVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "ConstDataValVar")
                {
                    ConstDataValVar* var = SVFUtil::dyn_cast<ConstDataValVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching ConstDataValVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFValVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "RetValPN")
                {
                    RetValPN* var = SVFUtil::dyn_cast<RetValPN>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching RetValPN found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFValVarAtrributes(properties, var, pag);
                    int call_graph_node_id = cJSON_GetObjectItem(properties, "call_graph_node_id")->valueint;
                    FunObjVar* callGraphNode = id2funObjVarsMap[call_graph_node_id];
                    if (nullptr != callGraphNode)
                    {
                        var->setCallGraphNode(callGraphNode);
                    }
                    else
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching FunObjVar found for id: " << call_graph_node_id <<" when updating RetValPN:"<<id<< "\n";
                    }
                }
                else if (nodeType == "VarArgValPN")
                {
                    VarArgValPN* var = SVFUtil::dyn_cast<VarArgValPN>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching VarArgValPN found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFValVarAtrributes(properties, var, pag);
                    int call_graph_node_id = cJSON_GetObjectItem(properties, "call_graph_node_id")->valueint;
                    FunObjVar* callGraphNode = id2funObjVarsMap[call_graph_node_id];
                    if (nullptr != callGraphNode)
                    {
                        var->setCallGraphNode(callGraphNode);
                        pag->varargFunObjSymMap[callGraphNode] = var->getId();
                    }
                    else
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching FunObjVar found for id: " << call_graph_node_id <<" when updating VarArgValPN:"<<id<< "\n";
                    }
                }
                else if (nodeType == "DummyValVar")
                {
                    DummyValVar* var = SVFUtil::dyn_cast<DummyValVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching DummyValVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFValVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "ConstAggValVar")
                {
                    ConstAggValVar* var = SVFUtil::dyn_cast<ConstAggValVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching ConstAggValVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFValVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "GlobalValVar")
                {
                    GlobalValVar* var = SVFUtil::dyn_cast<GlobalValVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching GlobalValVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFValVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "FunValVar")
                {
                    FunValVar* var = SVFUtil::dyn_cast<FunValVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching FunValVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFValVarAtrributes(properties, var, pag);
                    int fun_obj_var_node_id = cJSON_GetObjectItem(properties, "fun_obj_var_node_id")->valueint;
                    FunObjVar* funObjVar = id2funObjVarsMap[fun_obj_var_node_id];
                    if (nullptr != funObjVar)
                    {
                        var->setFunction(funObjVar);
                    }
                    else
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching FunObjVar found for id: " << fun_obj_var_node_id <<" when updating FunValVar:"<<id<< "\n";
                    }
                }
                else if (nodeType == "GepValVar")
                {
                    GepValVar* var = SVFUtil::dyn_cast<GepValVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching GepValVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFValVarAtrributes(properties, var, pag);
                    updateGepValVarAttributes(properties, var, pag);
    
                }
                else if (nodeType == "ValVar")
                {
                    ValVar* var = SVFUtil::dyn_cast<ValVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching ValVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFValVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "ConstNullPtrObjVar")
                {
                    ConstNullPtrObjVar* var = SVFUtil::dyn_cast<ConstNullPtrObjVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching ConstNullPtrObjVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFBaseObjVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "ConstIntObjVar")
                {
                    ConstIntObjVar* var = SVFUtil::dyn_cast<ConstIntObjVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching ConstIntObjVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFBaseObjVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "ConstFPObjVar")
                {
                    ConstFPObjVar* var = SVFUtil::dyn_cast<ConstFPObjVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching ConstFPObjVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFBaseObjVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "ConstDataObjVar")
                {
                    ConstDataObjVar* var = SVFUtil::dyn_cast<ConstDataObjVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching ConstDataObjVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFBaseObjVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "DummyObjVar")
                {
                    DummyObjVar* var = SVFUtil::dyn_cast<DummyObjVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching DummyObjVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFBaseObjVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "ConstAggObjVar")
                {
                    ConstAggObjVar* var = SVFUtil::dyn_cast<ConstAggObjVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching ConstAggObjVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFBaseObjVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "GlobalObjVar")
                {
                    GlobalObjVar* var = SVFUtil::dyn_cast<GlobalObjVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching GlobalObjVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFBaseObjVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "FunObjVar")
                {
                    FunObjVar* var = SVFUtil::dyn_cast<FunObjVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching FunObjVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFBaseObjVarAtrributes(properties, var, pag);
                    updateFunObjVarAttributes(properties, var, pag);
                }
                else if (nodeType == "StackObjVar")
                {
                    StackObjVar* var = SVFUtil::dyn_cast<StackObjVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching StackObjVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFBaseObjVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "HeapObjVar")
                {
                    HeapObjVar* var = SVFUtil::dyn_cast<HeapObjVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching HeapObjVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFBaseObjVarAtrributes(properties, var, pag);
                }
                else if (nodeType == "BaseObjVar")
                {
                    BaseObjVar* var = SVFUtil::dyn_cast<BaseObjVar>(pag->getGNode(id));
                    if (var == nullptr)
                    {
                        SVFUtil::outs() << "Warning: [updateSVFPAGNodesAttributesFromDB] No matching BaseObjVar found for id: " << id << "\n";
                        continue;
                    }
                    updateSVFBaseObjVarAtrributes(properties, var, pag);
                }
                skip += 1;
            }
            cJSON_Delete(root);
        }

    }
}


void GraphDBClient::readPAGNodesFromDB(lgraph::RpcClient* connection, const std::string& dbname, std::string nodeType, SVFIR* pag)
{
    int skip = 0;
    int limit = 1000;
    while (true)
    {
        std::string result;
        std::string queryStatement = " MATCH (node:"+nodeType+") RETURN node SKIP "+std::to_string(skip)+" LIMIT "+std::to_string(limit);
        cJSON* root = queryFromDB(connection, dbname, queryStatement);
        if (nullptr == root)
        {
            break;
        }
        else
        {
            cJSON* node;
            cJSON_ArrayForEach(node, root)
            {
                cJSON* data = cJSON_GetObjectItem(node, "node");
                if (!data)
                    continue;
                cJSON* properties = cJSON_GetObjectItem(data, "properties");
                if (!properties)
                    continue;
                SVFVar* var = nullptr;
                int id = cJSON_GetObjectItem(properties,"id")->valueint;
                int svfTypeId = cJSON_GetObjectItem(properties, "svf_type_id")->valueint;
                const SVFType* type = pag->getSVFType(svfTypeId);
                if (type == nullptr)
                {
                    SVFUtil::outs() << "Warning: [readPAGNodesFromDB] No matching SVFType found for type: " << svfTypeId << "for PAGNode:"<<id<<"\n";
                }
    
                if (nodeType == "ConstNullPtrValVar")
                {
                    var = new ConstNullPtrValVar(id, type, ValVar::ConstNullptrValNode);
                    pag->addValNode(SVFUtil::cast<ConstNullPtrValVar>(var));
                    NodeIDAllocator::get()->increaseNumOfValues();
                }
                else if (nodeType == "ConstIntValVar")
                {
                    u64_t zval = std::stoull(cJSON_GetObjectItem(properties, "zval")->valuestring);
                    s64_t sval = cJSON_GetObjectItem(properties, "sval")->valueint;
                    var = new ConstIntValVar(id, sval, zval, type, ValVar::ConstIntValNode);
                    pag->addValNode(SVFUtil::cast<ConstIntValVar>(var));
                    NodeIDAllocator::get()->increaseNumOfValues();
                }
                else if (nodeType == "ConstFPValVar")
                {
                    double dval = cJSON_GetObjectItem(properties, "dval")->valuedouble;
                    var = new ConstFPValVar(id, dval, type, ValVar::ConstFPValNode);
                    pag->addValNode(SVFUtil::cast<ConstFPValVar>(var));
                    NodeIDAllocator::get()->increaseNumOfValues();
                }
                else if (nodeType == "ArgValVar")
                {
                    u32_t arg_no = static_cast<u32_t>(cJSON_GetObjectItem(properties, "arg_no")->valueint);
                    var = new ArgValVar(id, type,arg_no, ValVar::ArgValNode);
                    pag->addValNode(SVFUtil::cast<ArgValVar>(var));
                    NodeIDAllocator::get()->increaseNumOfValues();
                }
                else if (nodeType == "BlackHoleValVar")
                {
                    var = new BlackHoleValVar(id, type, ValVar::BlackHoleValNode);
                    pag->addValNode(SVFUtil::cast<BlackHoleValVar>(var));
                    NodeIDAllocator::get()->increaseNumOfValues();
                }
                else if (nodeType == "ConstDataValVar")
                {
                    var = new ConstDataValVar(id, type, ValVar::ConstDataValNode);
                    pag->addValNode(SVFUtil::cast<ConstDataValVar>(var));
                    NodeIDAllocator::get()->increaseNumOfValues();
                }
                else if (nodeType == "RetValPN")
                {
                    var = new RetValPN(id, type, ValVar::RetValNode);
                    pag->addValNode(SVFUtil::cast<RetValPN>(var));
                    NodeIDAllocator::get()->increaseNumOfValues();
                }
                else if (nodeType == "VarArgValPN")
                {
                    var = new VarArgValPN(id, type, ValVar::VarargValNode);
                    pag->addValNode(SVFUtil::cast<VarArgValPN>(var));
                    NodeIDAllocator::get()->increaseNumOfValues();
                }
                else if (nodeType == "DummyValVar")
                {
                    var = new DummyValVar(id, type, ValVar::DummyValNode);
                    pag->addValNode(SVFUtil::cast<DummyValVar>(var));
                    NodeIDAllocator::get()->increaseNumOfValues();
                }
                else if (nodeType == "ConstAggValVar")
                {
                    var = new ConstAggValVar(id, type, ValVar::ConstAggValNode);
                    pag->addValNode(SVFUtil::cast<ConstAggValVar>(var));
                    NodeIDAllocator::get()->increaseNumOfValues();
                }
                else if (nodeType == "GlobalValVar")
                {
                    var = new GlobalValVar(id, type, ValVar::GlobalValNode);
                    pag->addValNode(SVFUtil::cast<GlobalValVar>(var));
                    NodeIDAllocator::get()->increaseNumOfValues();
                }
                else if (nodeType == "FunValVar")
                {
                    var = new FunValVar(id, type, ValVar::FunValNode);
                    pag->addValNode(SVFUtil::cast<FunValVar>(var));
                    NodeIDAllocator::get()->increaseNumOfValues();
                }
                else if (nodeType == "GepValVar")
                {
                    int gep_val_svf_type_id = cJSON_GetObjectItem(properties, "gep_val_svf_type_id")->valueint;
                    const SVFType* gepValType = pag->getSVFType(gep_val_svf_type_id);
                    var = new GepValVar(id, type, gepValType, ValVar::GepValNode);
                    pag->addValNode(SVFUtil::cast<GepValVar>(var));
                    NodeIDAllocator::get()->increaseNumOfValues();
                }
                else if (nodeType == "ValVar")
                {
                    var = new ValVar(id, type, ValVar::ValNode);
                    pag->addValNode(SVFUtil::cast<ValVar>(var));
                    NodeIDAllocator::get()->increaseNumOfValues();
                }
                else if (nodeType == "ConstNullPtrObjVar")
                {
                    ObjTypeInfo* objTypeInfo = parseObjTypeInfoFromDB(properties, pag);
                    var = new ConstNullPtrObjVar(id, type, objTypeInfo, ObjVar::ConstNullptrObjNode);
                    pag->addBaseObjNode(SVFUtil::cast<ConstNullPtrObjVar>(var));
                    NodeIDAllocator::get()->increaseNumOfObjAndNodes();
                }
                else if (nodeType == "ConstIntObjVar")
                {
                    ObjTypeInfo* objTypeInfo = parseObjTypeInfoFromDB(properties, pag);
                    u64_t zval = std::stoull(cJSON_GetObjectItem(properties, "zval")->valuestring);
                    s64_t sval = cJSON_GetObjectItem(properties, "sval")->valueint;
                    var = new ConstIntObjVar(id, sval, zval, type, objTypeInfo, ObjVar::ConstIntObjNode);
                    pag->addBaseObjNode(SVFUtil::cast<ConstIntObjVar>(var));
                    NodeIDAllocator::get()->increaseNumOfObjAndNodes();
                }
                else if (nodeType == "ConstFPObjVar")
                {
                    ObjTypeInfo* objTypeInfo = parseObjTypeInfoFromDB(properties, pag);
                    float dval = (float)(cJSON_GetObjectItem(properties, "dval")->valuedouble);
                    var = new ConstFPObjVar(id, dval, type, objTypeInfo, ObjVar::ConstFPObjNode);
                    pag->addBaseObjNode(SVFUtil::cast<ConstFPObjVar>(var));
                    NodeIDAllocator::get()->increaseNumOfObjAndNodes();
                }
                else if (nodeType == "ConstDataObjVar")
                {
                    ObjTypeInfo* objTypeInfo = parseObjTypeInfoFromDB(properties, pag);
                    var = new ConstDataObjVar(id, type, objTypeInfo, ObjVar::ConstDataObjNode);
                    pag->addBaseObjNode(SVFUtil::cast<ConstDataObjVar>(var));
                    NodeIDAllocator::get()->increaseNumOfObjAndNodes();
                }
                else if (nodeType == "DummyObjVar")
                {
                    ObjTypeInfo* objTypeInfo = parseObjTypeInfoFromDB(properties, pag);
                    var = new DummyObjVar(id, type, objTypeInfo, ObjVar::DummyObjNode);
                    pag->addDummyObjNode(SVFUtil::cast<DummyObjVar>(var));
                    NodeIDAllocator::get()->increaseNumOfObjAndNodes();
                }
                else if (nodeType == "ConstAggObjVar")
                {
                    ObjTypeInfo* objTypeInfo = parseObjTypeInfoFromDB(properties, pag);
                    var = new ConstAggObjVar(id, type, objTypeInfo, ObjVar::ConstAggObjNode);
                    pag->addBaseObjNode(SVFUtil::cast<ConstAggObjVar>(var));
                    NodeIDAllocator::get()->increaseNumOfObjAndNodes();
                }
                else if (nodeType == "GlobalObjVar")
                {
                    ObjTypeInfo* objTypeInfo = parseObjTypeInfoFromDB(properties, pag);
                    var = new GlobalObjVar(id, type, objTypeInfo, ObjVar::GlobalObjNode);
                    std::string val_name = cJSON_GetObjectItem(properties, "val_name")->valuestring;
                    if (!val_name.empty())
                    {
                        var->setName(val_name);
                    }
                    pag->addBaseObjNode(SVFUtil::cast<GlobalObjVar>(var)); 
                    NodeIDAllocator::get()->increaseNumOfObjAndNodes();
                }
                else if (nodeType == "FunObjVar")
                {
                    ObjTypeInfo* objTypeInfo = parseObjTypeInfoFromDB(properties, pag);
                    bool is_decl = cJSON_IsTrue(cJSON_GetObjectItem(properties, "is_decl"));
                    bool intrinsic = cJSON_IsTrue(cJSON_GetObjectItem(properties, "intrinsic"));
                    bool is_addr_taken = cJSON_IsTrue(cJSON_GetObjectItem(properties, "is_addr_taken"));
                    bool is_uncalled = cJSON_IsTrue(cJSON_GetObjectItem(properties, "is_uncalled"));
                    bool is_not_return = cJSON_IsTrue(cJSON_GetObjectItem(properties, "is_not_ret"));
                    bool sup_var_arg = cJSON_IsTrue(cJSON_GetObjectItem(properties, "sup_var_arg"));
                    int fun_type_id = cJSON_GetObjectItem(properties, "fun_type_id")->valueint;
                    const SVFFunctionType* funcType = SVFUtil::dyn_cast<SVFFunctionType>(pag->getSVFType(fun_type_id));
                    var = new FunObjVar(id, type, objTypeInfo, is_decl, intrinsic, is_addr_taken, is_uncalled, is_not_return, sup_var_arg, funcType, ObjVar::FunObjNode);
                    FunObjVar* funObjVar = SVFUtil::cast<FunObjVar>(var);
                    std::string func_annotation = cJSON_GetObjectItem(properties, "func_annotation")->valuestring;
                    if (!func_annotation.empty())
                    {
                        std::vector<std::string> func_annotation_vector;
                        func_annotation_vector = deserializeAnnotations(func_annotation);
                        ExtAPI::getExtAPI()->setExtFuncAnnotations(funObjVar, func_annotation_vector);
                    }
                    std::string val_name = cJSON_GetObjectItem(properties, "val_name")->valuestring;
                    if (!val_name.empty())
                    {
                        funObjVar->setName(val_name);
                    }
                    std::string all_args_node_ids = cJSON_GetObjectItem(properties, "all_args_node_ids")->valuestring;
                    if (!all_args_node_ids.empty())
                    {
                        std::vector<int> all_args_node_ids_vec = parseElements2Container<std::vector<int>>(all_args_node_ids);
                        for (int arg_id : all_args_node_ids_vec)
                        {
                            ArgValVar* arg = SVFUtil::dyn_cast<ArgValVar>(pag->getGNode(arg_id));
                            if (arg != nullptr)
                            {
                                funObjVar->addArgument(arg);
                            } 
                            else 
                            {
                                SVFUtil::outs() << "Warning: [readPAGNodesFromDB] No matching ArgValVar found for id: " << arg_id << "\n";
                            }
                        }
                    }
                    pag->addBaseObjNode(funObjVar);
                    id2funObjVarsMap[id] = funObjVar;  
                    NodeIDAllocator::get()->increaseNumOfObjAndNodes();              
                }
                else if (nodeType == "StackObjVar")
                {
                    ObjTypeInfo* objTypeInfo = parseObjTypeInfoFromDB(properties, pag);
                    var = new StackObjVar(id, type, objTypeInfo, ObjVar::StackObjNode);
                    pag->addBaseObjNode(SVFUtil::cast<StackObjVar>(var));
                    NodeIDAllocator::get()->increaseNumOfObjAndNodes();
                }
                else if (nodeType == "HeapObjVar")
                {
                    ObjTypeInfo* objTypeInfo = parseObjTypeInfoFromDB(properties, pag);
                    var = new HeapObjVar(id, type, objTypeInfo, ObjVar::HeapObjNode);
                    pag->addBaseObjNode(SVFUtil::cast<HeapObjVar>(var));
                    NodeIDAllocator::get()->increaseNumOfObjAndNodes();
                }
                else if (nodeType == "BaseObjVar")
                {
                    ObjTypeInfo* objTypeInfo = parseObjTypeInfoFromDB(properties, pag);
                    var = new BaseObjVar(id, type, objTypeInfo, ObjVar::BaseObjNode);
                    pag->addBaseObjNode(SVFUtil::cast<BaseObjVar>(var));
                    NodeIDAllocator::get()->increaseNumOfObjAndNodes();
                }
                else if (nodeType == "GepObjVar")
                {
                    s64_t app_offset = cJSON_GetObjectItem(properties, "app_offset")->valueint;
                    int base_obj_var_node_id = cJSON_GetObjectItem(properties, "base_obj_var_node_id")->valueint;
                    const BaseObjVar* baseObj = pag->getBaseObject(base_obj_var_node_id);
                    var = new GepObjVar(id, type, app_offset, baseObj, ObjVar::GepObjNode);
                    pag->addGepObjNode(SVFUtil::cast<GepObjVar>(var), base_obj_var_node_id, app_offset);
                    NodeIDAllocator::get()->increaseNumOfObjAndNodes();
                }
                else if (nodeType == "ObjVar")
                {
                    var = new ObjVar(id, type, ObjVar::ObjNode);
                    pag->addObjNode(SVFUtil::cast<ObjVar>(var));
                    NodeIDAllocator::get()->increaseNumOfObjAndNodes();
                }

                std::string sourceLocation = parseNodeSourceLocation(node);
                if (var != nullptr && !sourceLocation.empty())
                {
                    var->setSourceLoc(sourceLocation);
                }
                skip += 1;
            }
            cJSON_Delete(root);
        }
    }
}

ObjTypeInfo* GraphDBClient::parseObjTypeInfoFromDB(cJSON* properties, SVFIR* pag)
{
    int obj_type_info_type_id = cJSON_GetObjectItem(properties, "obj_type_info_type_id")->valueint;
    const SVFType* objTypeInfoType = pag->getSVFType(obj_type_info_type_id);
    int obj_type_info_flags = cJSON_GetObjectItem(properties, "obj_type_info_flags")->valueint;
    int obj_type_info_max_offset_limit = cJSON_GetObjectItem(properties, "obj_type_info_max_offset_limit")->valueint;
    int obj_type_info_elem_num = cJSON_GetObjectItem(properties, "obj_type_info_elem_num")->valueint;
    int obj_type_info_byte_size = cJSON_GetObjectItem(properties, "obj_type_info_byte_size")->valueint;
    ObjTypeInfo* objTypeInfo = new ObjTypeInfo(objTypeInfoType, obj_type_info_flags, obj_type_info_max_offset_limit, obj_type_info_elem_num, obj_type_info_byte_size);
    if (nullptr != objTypeInfo)
        return objTypeInfo;
    return nullptr;
}

cJSON* GraphDBClient::queryFromDB(lgraph::RpcClient* connection, const std::string& dbname, std::string queryStatement)
{
    // parse all SVFType
    std::string result;
    if (!connection->CallCypher(result, queryStatement, dbname))
    {
        SVFUtil::outs() << queryStatement<< "\n";
        SVFUtil::outs() << "Failed to query from DB:" << result << "\n";
        return nullptr;
    } 
    cJSON* root = cJSON_Parse(result.c_str());
    if (!root || !cJSON_IsArray(root))
    {
        SVFUtil::outs() << "Invalid JSON format: "<<queryStatement<<"\n";
        cJSON_Delete(root);
        return nullptr;
    }
    // TODO: need to fix: all graph should support pagination query not only the PAG
    if (dbname != "BasicBlockGraph" && result=="[]")
    {
        SVFUtil::outs() << "No data found for query: " << queryStatement << "\n";
        cJSON_Delete(root);
        return nullptr;
    }

    return root;
}

void GraphDBClient::readBasicBlockGraphFromDB(lgraph::RpcClient* connection, const std::string& dbname)
{
    SVFUtil::outs()<< "Build BasicBlockGraph from DB....\n";
    for (auto& item : id2funObjVarsMap)
    {
        FunObjVar* funObjVar = item.second;
        readBasicBlockNodesFromDB(connection, dbname, funObjVar);
    }

    for (auto& item : id2funObjVarsMap)
    {
        FunObjVar* funObjVar = item.second;
        readBasicBlockEdgesFromDB(connection, dbname, funObjVar);
    }
}

void GraphDBClient::readBasicBlockNodesFromDB(lgraph::RpcClient* connection, const std::string& dbname, FunObjVar* funObjVar)
{
        NodeID funObjId = funObjVar->getId();
        std::string queryStatement ="MATCH (node) where node.fun_obj_var_id = " + std::to_string(funObjId) +" RETURN node ";
        cJSON* root = queryFromDB(connection, dbname, queryStatement);
        if (nullptr != root)
        {
            cJSON* node;
            BasicBlockGraph* bbGraph = new BasicBlockGraph();
            funObjVar->setBasicBlockGraph(bbGraph);
            cJSON_ArrayForEach(node, root)
            {
                cJSON* data = cJSON_GetObjectItem(node, "node");
                if (!data)
                    continue;
                cJSON* properties = cJSON_GetObjectItem(data, "properties");
                if (!properties)
                    continue;
                std::string id = cJSON_GetObjectItem(properties, "id")->valuestring;
                std::string bb_name =
                    cJSON_GetObjectItem(properties, "bb_name")->valuestring;
                int bbId = parseBBId(id);
                SVFBasicBlock* bb = new SVFBasicBlock(bbId, funObjVar);
                bb->setName(bb_name);
                bbGraph->addBasicBlock(bb);
                basicBlocks.insert(bb);
                std::string allICFGNodeIds = cJSON_GetObjectItem(properties, "all_icfg_nodes_ids")->valuestring;
                if (!allICFGNodeIds.empty())
                    bb2AllICFGNodeIdstrMap.insert(std::make_pair(bb, allICFGNodeIds));
            }
            cJSON_Delete(root);
        }   
    
}

void GraphDBClient::updateBasicBlockNodes(ICFG* icfg)
{
    for (auto& item:bb2AllICFGNodeIdstrMap)
    {
        SVFBasicBlock* bb = item.first;
        std::string allICFGNodeIds = item.second;
        if (!allICFGNodeIds.empty())
        {
            std::vector<int> allICFGNodeIdsVec = parseElements2Container<std::vector<int>>(allICFGNodeIds);
            for (int icfgId : allICFGNodeIdsVec)
            {
                ICFGNode* icfgNode = icfg->getICFGNode(icfgId);
                if (icfgNode != nullptr)
                {
                    bb->addICFGNode(icfgNode);
                }
                else
                {
                    SVFUtil::outs() << "Warning: [updateBasicBlockNodes] No matching ICFGNode found for id: " << icfgId << "\n";
                }
            }
        }
    }
}

void GraphDBClient::readBasicBlockEdgesFromDB(lgraph::RpcClient* connection, const std::string& dbname, FunObjVar* funObjVar)
{
    BasicBlockGraph* bbGraph = funObjVar->getBasicBlockGraph();
    if (nullptr != bbGraph)
    {
        for (auto& pair: *bbGraph)
        {
                SVFBasicBlock* bb = pair.second;
                std::string queryStatement = "MATCH (node{id:'"+std::to_string(bb->getId())+":"+std::to_string(bb->getFunction()->getId())+"'}) RETURN node.pred_bb_ids, node.sscc_bb_ids ";
                cJSON* root = queryFromDB(connection, dbname, queryStatement);
                if (nullptr != root)
                {
                    cJSON* item;
                    cJSON_ArrayForEach(item, root)
                    {
                        if (!item)
                            continue;
                        std::string pred_bb_ids = cJSON_GetObjectItem(item, "node.pred_bb_ids")->valuestring;
                        std::string sscc_bb_ids = cJSON_GetObjectItem(item, "node.sscc_bb_ids")->valuestring;
                        if (!pred_bb_ids.empty())
                        {
                            std::vector<int> predBBIds = parseElements2Container<std::vector<int>>(pred_bb_ids);
                            for (int predBBId : predBBIds)
                            {
    
                                SVFBasicBlock* predBB = bbGraph->getGNode(predBBId);
                                if (nullptr != predBB)
                                {
                                    bb->addPredBasicBlock(predBB);
                                }
    
                            }
                        }
                        if (!sscc_bb_ids.empty())
                        {
                            std::vector<int> ssccBBIds = parseElements2Container<std::vector<int>>(sscc_bb_ids);
                            for (int ssccBBId : ssccBBIds)
                            {
                                SVFBasicBlock* ssccBB = bbGraph->getGNode(ssccBBId);
                                if (nullptr != ssccBB)
                                {
                                    bb->addSuccBasicBlock(ssccBB);
                                }
    
                            }
                        }
                    }
                    cJSON_Delete(root);
                }
                    
        }
    }
}

ICFG* GraphDBClient::buildICFGFromDB(lgraph::RpcClient* connection, const std::string& dbname, SVFIR* pag)
{
    SVFUtil::outs()<< "Build ICFG from DB....\n";
    DBOUT(DGENERAL, outs() << pasMsg("\t Building ICFG From DB ...\n"));
    ICFG* icfg = new ICFG();
    // read & add all the ICFG nodes from DB
    readICFGNodesFromDB(connection, dbname, "GlobalICFGNode", icfg, pag);
    readICFGNodesFromDB(connection, dbname, "FunEntryICFGNode", icfg, pag);
    readICFGNodesFromDB(connection, dbname, "FunExitICFGNode", icfg, pag);
    readICFGNodesFromDB(connection, dbname, "IntraICFGNode", icfg, pag);
    // need to parse the RetICFGNode first before parsing the CallICFGNode
    readICFGNodesFromDB(connection, dbname, "RetICFGNode", icfg, pag);
    readICFGNodesFromDB(connection, dbname, "CallICFGNode", icfg, pag);

    // read & add all the ICFG edges from DB
    readICFGEdgesFromDB(connection, dbname, "IntraCFGEdge", icfg, pag);
    readICFGEdgesFromDB(connection, dbname, "CallCFGEdge", icfg, pag);
    readICFGEdgesFromDB(connection, dbname, "RetCFGEdge", icfg, pag);

    updateBasicBlockNodes(icfg);

    return icfg;
}

void GraphDBClient::readICFGNodesFromDB(lgraph::RpcClient* connection, const std::string& dbname, std::string nodeType, ICFG* icfg, SVFIR* pag)
{
    int skip = 0;
    int limit = 1000;
    while (true)
    {
        std::string queryStatement = " MATCH (node:"+nodeType+") RETURN node SKIP "+std::to_string(skip)+" LIMIT "+std::to_string(limit);
        cJSON* root = queryFromDB(connection, dbname, queryStatement);
        if (nullptr == root)
        {
            break;
        }
        else
        {
            cJSON* node;
            cJSON_ArrayForEach(node, root)
            {
                ICFGNode* icfgNode = nullptr;
                if (nodeType == "GlobalICFGNode")
                {
                    icfgNode = parseGlobalICFGNodeFromDBResult(node, pag);
                    if (nullptr != icfgNode)
                    {
                        icfg->addGlobalICFGNode(SVFUtil::cast<GlobalICFGNode>(icfgNode));
                    }
                }
                else if (nodeType == "IntraICFGNode")
                {
                    icfgNode = parseIntraICFGNodeFromDBResult(node, pag);
                    if (nullptr != icfgNode)
                    {
                        icfg->addICFGNode(icfgNode);
                    }
                }
                else if (nodeType == "FunEntryICFGNode")
                {
                    icfgNode = parseFunEntryICFGNodeFromDBResult(node, pag);
                    if (nullptr != icfgNode)
                    {
                        icfg->addFunEntryICFGNode(SVFUtil::cast<FunEntryICFGNode>(icfgNode));
                    }
                }
                else if (nodeType == "FunExitICFGNode")
                {
                    icfgNode = parseFunExitICFGNodeFromDBResult(node, pag);
                    if (nullptr != icfgNode)
                    {
                        icfg->addFunExitICFGNode(SVFUtil::cast<FunExitICFGNode>(icfgNode));
                    }
                }
                else if (nodeType == "RetICFGNode")
                {
                    icfgNode = parseRetICFGNodeFromDBResult(node, pag);
                    if (nullptr != icfgNode)
                    {
                        icfg->addICFGNode(icfgNode);
                        id2RetICFGNodeMap[icfgNode->getId()] = SVFUtil::cast<RetICFGNode>(icfgNode);
                    }
                }
                else if (nodeType == "CallICFGNode")
                {
                    icfgNode = parseCallICFGNodeFromDBResult(node, pag);
                    if (nullptr != icfgNode)
                    {
                        CallICFGNode* callNode = SVFUtil::cast<CallICFGNode>(icfgNode);
                        icfg->addICFGNode(callNode);
                        if (callNode->isIndirectCall())
                        {
                            pag->addIndirectCallsites(callNode, callNode->getIndFunPtr()->getId());
                            // SVFUtil::outs() << "Added indirect call site for node: " << callNode->getId() << "\n";
                            pag->addCallSite(callNode);
                        }
                        else 
                        {
                            const FunObjVar* calledFunc = callNode->getCalledFunction();
                            if (calledFunc != nullptr && calledFunc->isIntrinsic() == false )
                            {
                                // SVFUtil::outs() << "Added direct call site for node: " << callNode->getId() << "\n";
                                pag->addCallSite(callNode);
                            }
                        }
                    }
                }

                std::string sourceLocation = parseNodeSourceLocation(node);
                if (!sourceLocation.empty())
                {
                    icfgNode->setSourceLoc(sourceLocation);
                }
                
                if (nullptr == icfgNode)
                {
                    SVFUtil::outs()<< "Failed to create "<< nodeType<< " from db query result\n";
                }
                skip += 1;
            }
            cJSON_Delete(root);
        }
    }
}

void GraphDBClient::parseSVFStmtsForICFGNodeFromDBResult(SVFIR* pag)
{
    if (!icfgNode2StmtsStrMap.empty())
    {
        for (auto& pair : icfgNode2StmtsStrMap)
        {
            ICFGNode* icfgNode = pair.first;
            std::string svfStmtIds = pair.second;
            if (!svfStmtIds.empty())
            {
                std::list<int> svfStmtIdsVec = parseElements2Container<std::list<int>>(svfStmtIds);
                for (int stmtId : svfStmtIdsVec)
                {
                    SVFStmt* stmt = edgeId2SVFStmtMap[stmtId];
                    if (stmt != nullptr)
                    {
                        pag->addToSVFStmtList(icfgNode,stmt);
                        icfgNode->addSVFStmt(stmt);
                    }
                    else
                    {
                        SVFUtil::outs() << "Warning: [parseSVFStmtsForICFGNodeFromDBResult] No matching SVFStmt found for id: " << stmtId << "\n";
                    }
                }
            }
        }
    }
}

ICFGNode* GraphDBClient::parseGlobalICFGNodeFromDBResult(const cJSON* node, SVFIR* pag)
{
    cJSON* data = cJSON_GetObjectItem(node, "node");
    if (!data)
        return nullptr;

    cJSON* properties = cJSON_GetObjectItem(data, "properties");
    if (!properties)
        return nullptr;

    GlobalICFGNode* icfgNode;
    int id = cJSON_GetObjectItem(properties,"id")->valueint;

    icfgNode = new GlobalICFGNode(id);
    std::string svfStmtIds = cJSON_GetObjectItem(properties, "pag_edge_ids")->valuestring;
    if (!svfStmtIds.empty())
    {
        icfgNode2StmtsStrMap[icfgNode] = svfStmtIds;
    }

    cJSON* dataNode = cJSON_GetObjectItem(properties, "chnodes_ids");
    std::string chnodes_ids = "";
    if (nullptr != dataNode->valuestring)
        chnodes_ids = dataNode->valuestring;
    if(!chnodes_ids.empty())
    {
        Set<int> chnodesIds = parseElements2Container<Set<int> >(chnodes_ids);
        updateCallNode2ClassesMap(icfgNode, chnodesIds, SVFUtil::cast<CHGraph>(pag->getCHG()));
    }
    dataNode = cJSON_GetObjectItem(properties, "cha_vtbls_ids");
    std::string cha_vtbls_ids = "";
    if (nullptr != dataNode->valuestring)
        cha_vtbls_ids = dataNode->valuestring;
    if(!cha_vtbls_ids.empty())
    {
        Set<int> chaVTblsIds = parseElements2Container<Set<int> >(cha_vtbls_ids);
        updateCallNode2CHAVtblsMap(icfgNode, chaVTblsIds, pag);
    }
    return icfgNode;
}

ICFGNode* GraphDBClient::parseFunEntryICFGNodeFromDBResult(const cJSON* node, SVFIR* pag)
{
    cJSON* data = cJSON_GetObjectItem(node, "node");
    if (!data)
        return nullptr;

    cJSON* properties = cJSON_GetObjectItem(data, "properties");
    if (!properties)
        return nullptr;

    FunEntryICFGNode* icfgNode;
    int id = cJSON_GetObjectItem(properties,"id")->valueint;
    int fun_obj_var_id = cJSON_GetObjectItem(properties, "fun_obj_var_id")->valueint; 
    FunObjVar* funObjVar = nullptr;
    auto funObjVarIt = id2funObjVarsMap.find(fun_obj_var_id);
    if (funObjVarIt != id2funObjVarsMap.end())
    {
        funObjVar = funObjVarIt->second;
    }
    else
    {
        SVFUtil::outs() << "Warning: [parseFunEntryICFGNodeFromDBResult] No matching FunObjVar found for id: " << fun_obj_var_id << "\n";
    }

    // parse FunEntryICFGNode bb
    int bb_id = cJSON_GetObjectItem(properties, "bb_id")->valueint;
    SVFBasicBlock* bb = funObjVar->getBasicBlockGraph()->getGNode(bb_id);

    icfgNode = new FunEntryICFGNode(id, funObjVar, bb);
    std::string fpNodesStr = cJSON_GetObjectItem(properties, "fp_nodes")->valuestring;
    std::vector<u32_t> fpNodesIdVec = parseElements2Container<std::vector<u32_t>>(fpNodesStr);
    for (auto fpNodeId: fpNodesIdVec)
    {
        SVFVar* fpNode = pag->getGNode(fpNodeId);
        if (nullptr != fpNode)
        {
            pag->addFunArgs(SVFUtil::cast<FunEntryICFGNode>(icfgNode), funObjVar, fpNode);
        }
        else 
        {
            SVFUtil::outs() << "Warning: [parseFunEntryICFGNodeFromDBResult] No matching fpNode SVFVar found for id: " << fpNodeId << "\n";
        }
    }

    std::string svfStmtIds = cJSON_GetObjectItem(properties, "pag_edge_ids")->valuestring;
    if (!svfStmtIds.empty())
    {
        icfgNode2StmtsStrMap[icfgNode] = svfStmtIds;
    }
    
    cJSON* dataNode = cJSON_GetObjectItem(properties, "chnodes_ids");
    std::string chnodes_ids = "";
    if (nullptr != dataNode->valuestring)
        chnodes_ids = dataNode->valuestring;
    if(!chnodes_ids.empty())
    {
        Set<int> chnodesIds = parseElements2Container<Set<int> >(chnodes_ids);
        updateCallNode2ClassesMap(icfgNode, chnodesIds, SVFUtil::cast<CHGraph>(pag->getCHG()));
    }
    dataNode = cJSON_GetObjectItem(properties, "cha_vtbls_ids");
    std::string cha_vtbls_ids = "";
    if (nullptr != dataNode->valuestring)
        cha_vtbls_ids = dataNode->valuestring;
    if(!cha_vtbls_ids.empty())
    {
        Set<int> chaVTblsIds = parseElements2Container<Set<int> >(cha_vtbls_ids);
        updateCallNode2CHAVtblsMap(icfgNode, chaVTblsIds, pag);
    }
    return icfgNode;
}

ICFGNode* GraphDBClient::parseFunExitICFGNodeFromDBResult(const cJSON* node, SVFIR* pag)
{
    cJSON* data = cJSON_GetObjectItem(node, "node");
    if (!data)
        return nullptr;

    cJSON* properties = cJSON_GetObjectItem(data, "properties");
    if (!properties)
        return nullptr;

    FunExitICFGNode* icfgNode;
    int id = cJSON_GetObjectItem(properties,"id")->valueint;

    int fun_obj_var_id = cJSON_GetObjectItem(properties, "fun_obj_var_id")->valueint;
    FunObjVar* funObjVar = nullptr;
    auto funObjVarIt = id2funObjVarsMap.find(fun_obj_var_id);
    if (funObjVarIt != id2funObjVarsMap.end())
    {
        funObjVar = funObjVarIt->second;
    }
    else
    {
        SVFUtil::outs() << "Warning: [parseFunExitICFGNodeFromDBResult] No matching FunObjVar found for id: " << fun_obj_var_id << "\n";
    }

    // parse FunExitICFGNode bb
    int bb_id = cJSON_GetObjectItem(properties, "bb_id")->valueint;
    SVFBasicBlock* bb = funObjVar->getBasicBlockGraph()->getGNode(bb_id);

    icfgNode = new FunExitICFGNode(id, funObjVar, bb);
    int formal_ret_node_id = cJSON_GetObjectItem(properties, "formal_ret_node_id")->valueint;
    if (formal_ret_node_id != -1)
    {
        SVFVar* formalRet = pag->getGNode(formal_ret_node_id);
        if (nullptr != formalRet)
        {
            pag->addFunRet(SVFUtil::cast<FunExitICFGNode>(icfgNode), funObjVar, formalRet);
        }
        else
        {
            SVFUtil::outs() << "Warning: [parseFunExitICFGNodeFromDBResult] No matching formalRet SVFVar found for id: " << formal_ret_node_id << "\n";
        }
    }

    std::string svfStmtIds = cJSON_GetObjectItem(properties, "pag_edge_ids")->valuestring;
    if (!svfStmtIds.empty())
    {
        icfgNode2StmtsStrMap[icfgNode] = svfStmtIds;
    }
    
    cJSON* dataNode = cJSON_GetObjectItem(properties, "chnodes_ids");
    std::string chnodes_ids = "";
    if (nullptr != dataNode->valuestring)
        chnodes_ids = dataNode->valuestring;
    if(!chnodes_ids.empty())
    {
        Set<int> chnodesIds = parseElements2Container<Set<int> >(chnodes_ids);
        updateCallNode2ClassesMap(icfgNode, chnodesIds, SVFUtil::cast<CHGraph>(pag->getCHG()));
    }
    dataNode = cJSON_GetObjectItem(properties, "cha_vtbls_ids");
    std::string cha_vtbls_ids = "";
    if (nullptr != dataNode->valuestring)
        cha_vtbls_ids = dataNode->valuestring;
    if(!cha_vtbls_ids.empty())
    {
        Set<int> chaVTblsIds = parseElements2Container<Set<int> >(cha_vtbls_ids);
        updateCallNode2CHAVtblsMap(icfgNode, chaVTblsIds, pag);
    }    return icfgNode;
}

ICFGNode* GraphDBClient::parseIntraICFGNodeFromDBResult(const cJSON* node, SVFIR* pag)
{
    cJSON* data = cJSON_GetObjectItem(node, "node");
    if (!data)
        return nullptr;

    cJSON* properties = cJSON_GetObjectItem(data, "properties");
    if (!properties)
        return nullptr;

    IntraICFGNode* icfgNode;
    int id = cJSON_GetObjectItem(properties, "id")->valueint;
    // parse intraICFGNode funObjVar
    int fun_obj_var_id = cJSON_GetObjectItem(properties, "fun_obj_var_id")->valueint;
    FunObjVar* funObjVar = nullptr;
    auto funObjVarIt = id2funObjVarsMap.find(fun_obj_var_id);
    if (funObjVarIt != id2funObjVarsMap.end())
    {
        funObjVar = funObjVarIt->second;
    }
    else
    {
        SVFUtil::outs() << "Warning: [parseIntraICFGNodeFromDBResult] No matching FunObjVar found for id: " << fun_obj_var_id << "\n";
    }

    // parse intraICFGNode bb
    int bb_id = cJSON_GetObjectItem(properties, "bb_id")->valueint;
    SVFBasicBlock* bb = funObjVar->getBasicBlockGraph()->getGNode(bb_id);

    // parse isRet 
    bool is_return = cJSON_IsTrue(cJSON_GetObjectItem(properties, "is_return"));

    
    icfgNode = new IntraICFGNode(id, bb, funObjVar, is_return);
    
    std::string svfStmtIds = cJSON_GetObjectItem(properties, "pag_edge_ids")->valuestring;
    if (!svfStmtIds.empty())
    {
        icfgNode2StmtsStrMap[icfgNode] = svfStmtIds;
    }
        
    cJSON* dataNode = cJSON_GetObjectItem(properties, "chnodes_ids");
    std::string chnodes_ids = "";
    if (nullptr != dataNode->valuestring)
        chnodes_ids = dataNode->valuestring;
    if(!chnodes_ids.empty())
    {
        Set<int> chnodesIds = parseElements2Container<Set<int> >(chnodes_ids);
        updateCallNode2ClassesMap(icfgNode, chnodesIds, SVFUtil::cast<CHGraph>(pag->getCHG()));
    }
    dataNode = cJSON_GetObjectItem(properties, "cha_vtbls_ids");
    std::string cha_vtbls_ids = "";
    if (nullptr != dataNode->valuestring)
        cha_vtbls_ids = dataNode->valuestring;
    if(!cha_vtbls_ids.empty())
    {
        Set<int> chaVTblsIds = parseElements2Container<Set<int> >(cha_vtbls_ids);
        updateCallNode2CHAVtblsMap(icfgNode, chaVTblsIds, pag);
    }    return icfgNode;
}

ICFGNode* GraphDBClient::parseRetICFGNodeFromDBResult(const cJSON* node, SVFIR* pag)
{
    cJSON* data = cJSON_GetObjectItem(node, "node");
    if (!data)
        return nullptr;

    cJSON* properties = cJSON_GetObjectItem(data, "properties");
    if (!properties)
        return nullptr;
    
    RetICFGNode* icfgNode;
    // parse retICFGNode id
    int id = cJSON_GetObjectItem(properties, "id")->valueint;

    // parse retICFGNode funObjVar
    int fun_obj_var_id = cJSON_GetObjectItem(properties, "fun_obj_var_id")->valueint;
    FunObjVar* funObjVar = nullptr;
    auto funObjVarIt = id2funObjVarsMap.find(fun_obj_var_id);
    if (funObjVarIt != id2funObjVarsMap.end())
    {
        funObjVar = funObjVarIt->second;
    }
    else
    {
        SVFUtil::outs() << "Warning: [parseRetICFGNodeFromDBResult] No matching FunObjVar found for id: " << fun_obj_var_id << "\n";
    }

    // parse retICFGNode bb
    int bb_id = cJSON_GetObjectItem(properties, "bb_id")->valueint;
    SVFBasicBlock* bb = funObjVar->getBasicBlockGraph()->getGNode(bb_id);

    // parse retICFGNode svfType
    int svfTypeId = cJSON_GetObjectItem(properties, "svf_type_id")->valueint;
    const SVFType* type = pag->getSVFType(svfTypeId);
    if (nullptr == type)
    {
        SVFUtil::outs() << "Warning: [parseRetICFGNodeFromDBResult] No matching SVFType found for: " << svfTypeId << "\n";
    }

    // create RetICFGNode Instance 
    icfgNode = new RetICFGNode(id, type, bb, funObjVar);

    // parse & add actualRet for RetICFGNode
    int actual_ret_node_id = cJSON_GetObjectItem(properties, "actual_ret_node_id")->valueint;
    if (actual_ret_node_id != -1)
    {
        SVFVar* actualRet = pag->getGNode(actual_ret_node_id);
        if (nullptr != actualRet)
        {
            pag->addCallSiteRets(SVFUtil::cast<RetICFGNode>(icfgNode), actualRet);
        }
        else
        {
            SVFUtil::outs() << "Warning: [parseRetICFGNodeFromDBResult] No matching actualRet SVFVar found for id: " << actual_ret_node_id << "\n";
        }
    }

    std::string svfStmtIds = cJSON_GetObjectItem(properties, "pag_edge_ids")->valuestring;
    if (!svfStmtIds.empty())
    {
        icfgNode2StmtsStrMap[icfgNode] = svfStmtIds;
    }
    
    cJSON* dataNode = cJSON_GetObjectItem(properties, "chnodes_ids");
    std::string chnodes_ids = "";
    if (nullptr != dataNode->valuestring)
        chnodes_ids = dataNode->valuestring;
    if(!chnodes_ids.empty())
    {
        Set<int> chnodesIds = parseElements2Container<Set<int> >(chnodes_ids);
        updateCallNode2ClassesMap(icfgNode, chnodesIds, SVFUtil::cast<CHGraph>(pag->getCHG()));
    }
    dataNode = cJSON_GetObjectItem(properties, "cha_vtbls_ids");
    std::string cha_vtbls_ids = "";
    if (nullptr != dataNode->valuestring)
        cha_vtbls_ids = dataNode->valuestring;
    if(!cha_vtbls_ids.empty())
    {
        Set<int> chaVTblsIds = parseElements2Container<Set<int> >(cha_vtbls_ids);
        updateCallNode2CHAVtblsMap(icfgNode, chaVTblsIds, pag);
    }    return icfgNode;
}

ICFGNode* GraphDBClient::parseCallICFGNodeFromDBResult(const cJSON* node, SVFIR* pag)
{
    cJSON* data = cJSON_GetObjectItem(node, "node");
    if (!data)
        return nullptr;

    cJSON* properties = cJSON_GetObjectItem(data, "properties");
    if (!properties)
        return nullptr;
    
    CallICFGNode* icfgNode;

    // parse CallICFGNode id
    int id = cJSON_GetObjectItem(properties, "id")->valueint;

    // parse CallICFGNode funObjVar
    int fun_obj_var_id = cJSON_GetObjectItem(properties, "fun_obj_var_id")->valueint;
    FunObjVar* funObjVar = nullptr;
    auto funObjVarIt = id2funObjVarsMap.find(fun_obj_var_id);
    if (funObjVarIt != id2funObjVarsMap.end())
    {
        funObjVar = funObjVarIt->second;
    }
    else
    {
        SVFUtil::outs() << "Warning: [parseCallICFGNodeFromDBResult] No matching FunObjVar found for id: " << fun_obj_var_id << "\n";
    }

    // parse CallICFGNode bb
    int bb_id = cJSON_GetObjectItem(properties, "bb_id")->valueint;
    SVFBasicBlock* bb = funObjVar->getBasicBlockGraph()->getGNode(bb_id);

    // parse CallICFGNode svfType
    int svfTypeId = cJSON_GetObjectItem(properties, "svf_type_id")->valueint;
    const SVFType* type = pag->getSVFType(svfTypeId);
    if (nullptr == type)
    {
        SVFUtil::outs() << "Warning: [parseCallICFGNodeFromDBResult] No matching SVFType found for: " << svfTypeId << "\n";
    }

    // parse CallICFGNode calledFunObjVar
    int called_fun_obj_var_id = cJSON_GetObjectItem(properties, "called_fun_obj_var_id")->valueint;
    FunObjVar* calledFunc = nullptr;
    if (called_fun_obj_var_id != -1)
    {
        auto calledFuncIt = id2funObjVarsMap.find(called_fun_obj_var_id);
        if (calledFuncIt != id2funObjVarsMap.end())
        {
            calledFunc = calledFuncIt->second;
        }
        else
        {
            SVFUtil::outs() << "Warning: [parseCallICFGNodeFromDBResult] No matching calledFunObjVar found for id: " << called_fun_obj_var_id << "\n";
        }
    }

    bool is_vararg = cJSON_IsTrue(cJSON_GetObjectItem(properties, "is_vararg"));
    bool is_vir_call_inst = cJSON_IsTrue(cJSON_GetObjectItem(properties, "is_vir_call_inst"));

    // parse CallICFGNode retICFGNode
    int ret_icfg_node_id = cJSON_GetObjectItem(properties, "ret_icfg_node_id")->valueint;
    RetICFGNode* retICFGNode = nullptr;
    if (ret_icfg_node_id != -1)
    {
        auto retICFGNodeIt = id2RetICFGNodeMap.find(ret_icfg_node_id);
        if (retICFGNodeIt != id2RetICFGNodeMap.end())
        {
            retICFGNode = retICFGNodeIt->second;
        }
        else
        {
            SVFUtil::outs() << "Warning: [parseCallICFGNodeFromDBResult] No matching RetICFGNode found for id: " << ret_icfg_node_id << "\n";
        }
    }
    
    std::string fun_name_of_v_call = "";
    s32_t virtualFunIdx = 0;
    SVFVar* vtabPtr = nullptr;
    if (is_vir_call_inst)
    {
        int virtual_fun_idx = cJSON_GetObjectItem(properties, "virtual_fun_idx")->valueint;
        virtualFunIdx = static_cast<s32_t>(virtual_fun_idx);
        int vtab_ptr_node_id = cJSON_GetObjectItem(properties, "vtab_ptr_node_id")->valueint;
        vtabPtr = pag->getGNode(vtab_ptr_node_id);
        fun_name_of_v_call = cJSON_GetObjectItem(properties, "fun_name_of_v_call")->valuestring;
    }
     
    // create CallICFGNode Instance
    icfgNode = new CallICFGNode(id, bb, type, funObjVar, calledFunc, retICFGNode,
        is_vararg, is_vir_call_inst, virtualFunIdx, vtabPtr, fun_name_of_v_call);

    int indFunPtrId = cJSON_GetObjectItem(properties, "ind_fun_ptr_var_id")->valueint;
    if (indFunPtrId != -1)
    {
        SVFVar* indFunPtr = pag->getGNode(indFunPtrId);
        if (nullptr != indFunPtr)
        {
            icfgNode->setIndFunPtr(indFunPtr);
        }
        else
        {
            SVFUtil::outs() << "Warning: [parseCallICFGNodeFromDBResult] No matching Indirect Function Pointer Var found for id: " << indFunPtrId << "\n";
        }
    }
    
    // parse CallICFGNode APNodes
    std::string ap_nodes = cJSON_GetObjectItem(properties, "ap_nodes")->valuestring;
    if (!ap_nodes.empty() && ap_nodes!= "[]")
    {
        std::vector<u32_t> apNodesIdVec = parseElements2Container<std::vector<u32_t>>(ap_nodes);
        if (apNodesIdVec.size() > 0)
        {
            for (auto apNodeId: apNodesIdVec)
            {
                SVFVar* apNode = pag->getGNode(apNodeId);
                if (nullptr != apNode)
                {
                    pag->addCallSiteArgs(SVFUtil::cast<CallICFGNode>(icfgNode), SVFUtil::cast<ValVar>(apNode));
                }
                else
                {
                    SVFUtil::outs() << "Warning: [parseCallICFGNodeFromDBResult] No matching APNode ValVar found for id: " << apNodeId << "\n";
                }
            }
        }
    }

    if (retICFGNode != nullptr)
    {
        retICFGNode->addCallBlockNodeFromDB(icfgNode);
    }
    
    std::string svfStmtIds = cJSON_GetObjectItem(properties, "pag_edge_ids")->valuestring;
    if (!svfStmtIds.empty())
    {
        icfgNode2StmtsStrMap[icfgNode] = svfStmtIds;
    }
    
    cJSON* dataNode = cJSON_GetObjectItem(properties, "chnodes_ids");
    std::string chnodes_ids = "";
    if (nullptr != dataNode->valuestring)
        chnodes_ids = dataNode->valuestring;
    if(!chnodes_ids.empty())
    {
        Set<int> chnodesIds = parseElements2Container<Set<int> >(chnodes_ids);
        updateCallNode2ClassesMap(icfgNode, chnodesIds, SVFUtil::cast<CHGraph>(pag->getCHG()));
    }
    dataNode = cJSON_GetObjectItem(properties, "cha_vtbls_ids");
    std::string cha_vtbls_ids = "";
    if (nullptr != dataNode->valuestring)
        cha_vtbls_ids = dataNode->valuestring;
    if(!cha_vtbls_ids.empty())
    {
        Set<int> chaVTblsIds = parseElements2Container<Set<int> >(cha_vtbls_ids);
        updateCallNode2CHAVtblsMap(icfgNode, chaVTblsIds, pag);
    }    return icfgNode;
}

void GraphDBClient::readICFGEdgesFromDB(lgraph::RpcClient* connection, const std::string& dbname, std::string edgeType, ICFG* icfg, SVFIR* pag)
{
    int skip = 0;
    int limit = 1000;
    while (true)
    {
        std::string queryStatement =  "MATCH ()-[edge:"+edgeType+"]->() RETURN edge SKIP "+std::to_string(skip)+" LIMIT "+std::to_string(limit);
        cJSON* root = queryFromDB(connection, dbname, queryStatement);
        if (nullptr == root)
        {
            break;
        }
        else
        {
            cJSON* edge;
            cJSON_ArrayForEach(edge, root)
            {
                ICFGEdge* icfgEdge = nullptr;
                if (edgeType == "IntraCFGEdge")
                {
                    icfgEdge = parseIntraCFGEdgeFromDBResult(edge, pag, icfg);
                }
                else if (edgeType == "CallCFGEdge")
                {
                    icfgEdge = parseCallCFGEdgeFromDBResult(edge, pag, icfg);
                }
                else if (edgeType == "RetCFGEdge")
                {
                    icfgEdge = parseRetCFGEdgeFromDBResult(edge, pag, icfg);
                }
                if (nullptr != icfgEdge)
                {
                    icfg->addICFGEdge(icfgEdge);
                }
                else 
                {
                    SVFUtil::outs()<< "Failed to create "<< edgeType << " from db query result\n";
                }
                skip += 1;
            }
            cJSON_Delete(root);
        }
    }
}

ICFGEdge* GraphDBClient::parseIntraCFGEdgeFromDBResult(const cJSON* edge, SVFIR* pag, ICFG* icfg)
{
    cJSON* data = cJSON_GetObjectItem(edge, "edge");
    if (!data)
        return nullptr;

    cJSON* properties = cJSON_GetObjectItem(data, "properties");
    if (!properties)
        return nullptr;

    IntraCFGEdge* icfgEdge;

    // parse srcICFGNode & dstICFGNode
    int src_id = cJSON_GetObjectItem(data,"src")->valueint;
    int dst_id = cJSON_GetObjectItem(data,"dst")->valueint; 
    ICFGNode* src = icfg->getICFGNode(src_id);
    ICFGNode* dst = icfg->getICFGNode(dst_id);

    if (src == nullptr)
    {
        SVFUtil::outs() << "Warning: [parseIntraCFGEdgeFromDBResult] No matching src ICFGNode found for id: " << src_id << "\n";
        return nullptr;
    }
    if (dst == nullptr)
    {
        SVFUtil::outs() << "Warning: [parseIntraCFGEdgeFromDBResult] No matching dst ICFGNode found for id: " << dst_id << "\n";
        return nullptr;
    }

    // create IntraCFGEdge Instance
    icfgEdge = new IntraCFGEdge(src, dst);
   
    // parse branchCondVal & conditionalVar
    int condition_var_id = cJSON_GetObjectItem(properties, "condition_var_id")->valueint;
    int branch_cond_val = cJSON_GetObjectItem(properties, "branch_cond_val")->valueint;
    s64_t branchCondVal = 0;
    SVFVar* conditionVar;
    if (condition_var_id != -1 && branch_cond_val != -1)
    {
        branchCondVal = static_cast<s64_t>(branch_cond_val);
        conditionVar = pag->getGNode(condition_var_id);
        if (nullptr == conditionVar)
        {
            SVFUtil::outs() << "Warning: [parseIntraCFGEdgeFromDBResult] No matching conditionVar found for id: " << condition_var_id << "\n";
        }
        icfgEdge->setConditionVar(conditionVar);
        icfgEdge->setBranchCondVal(branchCondVal);
    }

    return icfgEdge;
}

ICFGEdge* GraphDBClient::parseCallCFGEdgeFromDBResult(const cJSON* edge, SVFIR* pag, ICFG* icfg)
{
    cJSON* data = cJSON_GetObjectItem(edge, "edge");
    if (!data)
        return nullptr;

    cJSON* properties = cJSON_GetObjectItem(data, "properties");
    if (!properties)
        return nullptr;

    CallCFGEdge* icfgEdge;
    // parse srcICFGNode & dstICFGNode
    int src_id = cJSON_GetObjectItem(data,"src")->valueint;
    int dst_id = cJSON_GetObjectItem(data,"dst")->valueint; 
    ICFGNode* src = icfg->getICFGNode(src_id);
    ICFGNode* dst = icfg->getICFGNode(dst_id);
    if (src == nullptr)
    {
        SVFUtil::outs() << "Warning: [parseCallCFGEdgeFromDBResult] No matching src ICFGNode found for id: " << src_id << "\n";
        return nullptr;
    }
    if (dst == nullptr)
    {
        SVFUtil::outs() << "Warning: [parseCallCFGEdgeFromDBResult] No matching dst ICFGNode found for id: " << dst_id << "\n";
        return nullptr;
    }

    // create CallCFGEdge Instance
    icfgEdge = new CallCFGEdge(src, dst);
    std::string call_pe_ids = cJSON_GetObjectItem(properties, "call_pe_ids")->valuestring;
    if (!call_pe_ids.empty())
    {
        callCFGEdge2CallPEStrMap[icfgEdge] = call_pe_ids;
    }
    return icfgEdge;
}

ICFGEdge* GraphDBClient::parseRetCFGEdgeFromDBResult(const cJSON* edge, SVFIR* pag, ICFG* icfg)
{
    cJSON* data = cJSON_GetObjectItem(edge, "edge");
    if (!data)
        return nullptr;

    cJSON* properties = cJSON_GetObjectItem(data, "properties");
    if (!properties)
        return nullptr;
    
    RetCFGEdge* icfgEdge;
    // parse srcICFGNode & dstICFGNode
    int src_id = cJSON_GetObjectItem(data,"src")->valueint;
    int dst_id = cJSON_GetObjectItem(data,"dst")->valueint; 
    ICFGNode* src = icfg->getICFGNode(src_id);
    ICFGNode* dst = icfg->getICFGNode(dst_id);
    if (src == nullptr)
    {
        SVFUtil::outs() << "Warning: [parseRetCFGEdgeFromDBResult] No matching src ICFGNode found for id: " << src_id << "\n";
        return nullptr;
    }
    if (dst == nullptr)
    {
        SVFUtil::outs() << "Warning: [parseRetCFGEdgeFromDBResult] No matching dst ICFGNode found for id: " << dst_id << "\n";
        return nullptr;
    }

    // create RetCFGEdge Instance
    icfgEdge = new RetCFGEdge(src, dst);
    int ret_pe_id = cJSON_GetObjectItem(properties, "ret_pe_id")->valueint;
    if (ret_pe_id != -1)
    {
        retCFGEdge2RetPEStrMap[icfgEdge] = ret_pe_id;
    }
    return icfgEdge;
}

CHGraph* GraphDBClient::buildCHGraphFromDB(lgraph::RpcClient* connection, const std::string& dbname, SVFIR* pag)
{
    SVFUtil::outs()<< "Building CHGraph from DB....\n";
    DBOUT(DGENERAL, outs() << pasMsg("\t Building CHG From DB ...\n"));
    CHGraph* chg = new CHGraph();
    readCHNodesFromDB(connection, dbname, chg, pag);
    readCHEdgesFromDB(connection, dbname, chg);

    return chg;
}

void GraphDBClient::readCHNodesFromDB(lgraph::RpcClient* connection, const std::string& dbname, CHGraph* chg, SVFIR* pag)
{
    int skip = 0;
    int limit = 1000;
    while (true)
    {
        std::string queryStatement = " MATCH (node:CHNode) RETURN node SKIP "+std::to_string(skip)+" LIMIT "+std::to_string(limit);
        cJSON* root = queryFromDB(connection, dbname, queryStatement);
        if (nullptr == root)
        {
            break;
        }
        else
        {
            cJSON* node;
            cJSON_ArrayForEach(node, root)
            {
                parseCHNodeFromDB(node, chg, pag);
                skip += 1;
            }
            cJSON_Delete(root);
        }
    }
}

void GraphDBClient::parseCHNodeFromDB(const cJSON* node, CHGraph* chg, SVFIR* pag)
{
    cJSON* data = cJSON_GetObjectItem(node, "node");
    if (!data)
        return;

    cJSON* properties = cJSON_GetObjectItem(data, "properties");
    if (!properties)
        return;
    
    int id = cJSON_GetObjectItem(properties,"id")->valueint;
    std::string className = cJSON_GetObjectItem(properties, "class_name")->valuestring;
    
    // create new CHNode
    assert(!chg->getNode(className) && "this node should never be created before!");
    CHNode * chNode = new CHNode(className, id);
    // add CHNode to chg
    chg->classNameToNodeMap[className] = chNode;
    chg->addGNode(chNode->getId(), chNode);

    int vtable_id = cJSON_GetObjectItem(properties, "vtable_id")->valueint;
    int flags = cJSON_GetObjectItem(properties, "flags")->valueint;
    chNode->setFlag(static_cast<SVF::CHNode::CLASSATTR>(flags));
    if (vtable_id != -1)
    {
        GlobalObjVar* vtableVar = SVFUtil::dyn_cast<GlobalObjVar>(pag->getGNode(vtable_id));
        if (nullptr != vtableVar)
        {
            chNode->setVTable(vtableVar);
        }
        else
        {
            SVFUtil::outs() << "Warning: [parseCHNodeFromDB] No matching VTable Var found for id: " << vtable_id << "\n";
        }
    }

    std::string virtual_function_vectors = cJSON_GetObjectItem(properties, "virtual_function_vectors")->valuestring;
    std::vector<std::vector<const FunObjVar*>> virtualFunctionVectors = parseFuncVectorsFromString(virtual_function_vectors,pag);
    for (auto& funcVec : virtualFunctionVectors)
    {
        chNode->addVirtualFunctionVector(funcVec);
    }
}

void GraphDBClient::readCHEdgesFromDB(lgraph::RpcClient* connection, const std::string& dbname, CHGraph* chg)
{

}

CallGraph* GraphDBClient::buildCallGraphFromDB(lgraph::RpcClient* connection, const std::string& dbname, SVFIR* pag)
{
    SVFUtil::outs()<< "Build CallGraph from DB....\n";
    DBOUT(DGENERAL, outs() << pasMsg("\t Building CallGraph From DB ...\n"));
    CallGraph* callGraph = new CallGraph();
    readCallGraphNodesFromDB(connection, dbname, callGraph);
    readCallGraphEdgesFromDB(connection, dbname, pag, callGraph);

    return callGraph;
}

void GraphDBClient::readCallGraphNodesFromDB(lgraph::RpcClient* connection, const std::string& dbname, CallGraph* callGraph)
{
    int skip = 0;
    int limit = 1000;
    while (true)
    {
        std::string queryStatement = " MATCH (node:CallGraphNode) RETURN node SKIP "+std::to_string(skip)+" LIMIT "+std::to_string(limit);
        cJSON* root = queryFromDB(connection, dbname, queryStatement);
        if (nullptr == root)
        {
            break;
        }
        else
        {
            cJSON* node;
            cJSON_ArrayForEach(node, root)
            {
                CallGraphNode* cgNode = nullptr;
                cgNode = parseCallGraphNodeFromDB(node);
                if (nullptr != cgNode)
                {
                    callGraph->addCallGraphNode(cgNode);
                }
                skip += 1;
            }
            cJSON_Delete(root);
        }
    }
}

void GraphDBClient::readCallGraphEdgesFromDB(lgraph::RpcClient* connection, const std::string& dbname, SVFIR* pag, CallGraph* callGraph)
{
    int skip = 0;
    int limit = 1000;
    while (true)
    {
        std::string queryStatement = "MATCH ()-[edge]->() RETURN edge SKIP "+std::to_string(skip)+" LIMIT "+std::to_string(limit);
        cJSON* root = queryFromDB(connection, dbname, queryStatement);
        if (nullptr == root)
        {
            break;
        }
        else
        {
            cJSON* edge;
            cJSON_ArrayForEach(edge, root)
            {
                CallGraphEdge* cgEdge = nullptr;
                cgEdge = parseCallGraphEdgeFromDB(edge, pag, callGraph);
                if (nullptr != cgEdge)
                {
                    if (cgEdge->isDirectCallEdge())
                    {
                        callGraph->addDirectCallGraphEdge(cgEdge);
                    }
                    if (cgEdge->isIndirectCallEdge())
                    {
                        callGraph->addIndirectCallGraphEdge(cgEdge);
                    }
                }
                skip += 1;
            }
            cJSON_Delete(root);
        }
    }
}

CallGraphNode* GraphDBClient::parseCallGraphNodeFromDB(const cJSON* node)
{
    cJSON* data = cJSON_GetObjectItem(node, "node");
    if (!data)
        return nullptr;

    cJSON* properties = cJSON_GetObjectItem(data, "properties");
    if (!properties)
        return nullptr;
    
    int id = cJSON_GetObjectItem(properties,"id")->valueint;

    // parse funObjVar 
    int fun_obj_var_id = cJSON_GetObjectItem(properties, "fun_obj_var_id")->valueint;
    FunObjVar* funObjVar = nullptr;
    auto funObjVarIt = id2funObjVarsMap.find(fun_obj_var_id);
    if (funObjVarIt != id2funObjVarsMap.end())
    {
        funObjVar = funObjVarIt->second;
    }
    else
    {
        SVFUtil::outs() << "Warning: [parseCallGraphNodeFromDB] No matching FunObjVar found for id: " << fun_obj_var_id << "\n";
        return nullptr;
    }
    CallGraphNode* cgNode;

    // create callGraph node instance 
    cgNode = new CallGraphNode(id, funObjVar);

    std::string sourceLocation = cJSON_GetObjectItem(properties, "source_loc")->valuestring;
    if ( !sourceLocation.empty() )
    {
        cgNode->setSourceLoc(sourceLocation);
    }

    return cgNode;
}

CallGraphEdge* GraphDBClient::parseCallGraphEdgeFromDB(const cJSON* edge, SVFIR* pag, CallGraph* callGraph)
{
    CallGraphEdge* cgEdge = nullptr;
    cJSON* data = cJSON_GetObjectItem(edge, "edge");
    if (!data)
        return nullptr;

    cJSON* properties = cJSON_GetObjectItem(data, "properties");
    if (!properties)
        return nullptr;

    int src_id = cJSON_GetObjectItem(data,"src")->valueint;
    int dst_id = cJSON_GetObjectItem(data,"dst")->valueint;
    int csid = cJSON_GetObjectItem(properties,"csid")->valueint;
    std::string direct_call_set = cJSON_GetObjectItem(properties,"direct_call_set")->valuestring;
    std::string indirect_call_set = cJSON_GetObjectItem(properties, "indirect_call_set")->valuestring;
    int kind = cJSON_GetObjectItem(properties, "kind")->valueint;

    CallGraphNode* srcNode = callGraph->getGNode(src_id);
    CallGraphNode* dstNode = callGraph->getGNode(dst_id);
    if (srcNode == nullptr)
    {
        SVFUtil::outs() << "Warning: [parseCallGraphEdgeFromDB] No matching src CallGraphNode found for id: " << src_id << "\n";
        return nullptr;
    }
    if (dstNode == nullptr)
    {
        SVFUtil::outs() << "Warning: [parseCallGraphEdgeFromDB] No matching dst CallGraphNode found for id: " << dst_id << "\n";
        return nullptr;
    }

    // create CallGraphEdge Instance 
    cgEdge = new CallGraphEdge(srcNode, dstNode, static_cast<CallGraphEdge::CEDGEK>(kind), csid);
    Set<int> direct_call_set_ids;
    if (!direct_call_set.empty())
    {
        direct_call_set_ids = parseElements2Container<Set<int>>(direct_call_set);
        for (int directCallId : direct_call_set_ids)
        {
            CallICFGNode* node = SVFUtil::dyn_cast<CallICFGNode>(pag->getICFG()->getGNode(directCallId));
            callGraph->addCallSite(node, node->getCalledFunction(), cgEdge->getCallSiteID());
            cgEdge->addDirectCallSite(node);
            callGraph->callinstToCallGraphEdgesMap[node].insert(cgEdge);
        }
    }

    Set<int> indirect_call_set_ids;
    if (!indirect_call_set.empty())
    {
        indirect_call_set_ids = parseElements2Container<Set<int>>(indirect_call_set);
        for (int indirectCallId : indirect_call_set_ids)
        {
            CallICFGNode* node = SVFUtil::dyn_cast<CallICFGNode>(pag->getICFG()->getGNode(indirectCallId));
            callGraph->numOfResolvedIndCallEdge++;
            callGraph->addCallSite(node, node->getCalledFunction(), cgEdge->getCallSiteID());
            cgEdge->addInDirectCallSite(node);
            callGraph->callinstToCallGraphEdgesMap[node].insert(cgEdge);
        }
    }

    return cgEdge;
}

/// BasicBlockGraph insertions query statements
const std::string GraphDBClient::bbEdge2DBString(const BasicBlockEdge* edge)
{
    const std::string queryStatement =
    "MATCH (n:SVFBasicBlock {id:'"+std::to_string(edge->getSrcID())+":"+std::to_string(edge->getSrcNode()->getFunction()->getId())+
    "'}), (m:SVFBasicBlock{id:'"+std::to_string(edge->getDstID())+":"+std::to_string(edge->getDstNode()->getFunction()->getId())+
    "'}) WHERE n.id = '" +std::to_string(edge->getSrcID())+":" + std::to_string(edge->getSrcNode()->getFunction()->getId())+ "'"+
    " AND m.id = '" +std::to_string(edge->getDstID())+":" + std::to_string(edge->getDstNode()->getFunction()->getId())+ "'"+
    " CREATE (n)-[r:BasicBlockEdge{}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::bb2DBString(const SVFBasicBlock* bb)
{
    const std::string queryStatement ="CREATE (n:SVFBasicBlock {id:'" + std::to_string(bb->getId())+":" + std::to_string(bb->getFunction()->getId()) + "'" +
    ", fun_obj_var_id: " + std::to_string(bb->getFunction()->getId()) +
    ", bb_name:'" + bb->getName() +"'" +
    ", sscc_bb_ids:'" + extractNodesIds(bb->getSuccBBs()) + "'" +
    ", pred_bb_ids:'" + extractNodesIds(bb->getPredBBs()) + "'" +
    ", all_icfg_nodes_ids:'" + extractNodesIds(bb->getICFGNodeList()) + "'" +
    + "})";
    return queryStatement;
}

/// CallGraph insertions query statements
const std::string GraphDBClient::callGraphNode2DBString(const CallGraphNode* node)
{
    const std::string queryStatement ="CREATE (n:CallGraphNode {id: " + std::to_string(node->getId()) +
                             ", fun_obj_var_id: " + std::to_string(node->getFunction()->getId()) + 
                             node->sourceLocToDBString() +
                             ", fun_name:'" + node->getName()+ "'"
                             "})";
    return queryStatement;
}

const std::string GraphDBClient::callGraphEdge2DBString(const CallGraphEdge* edge)
{
    std::string indirectCallIds = "";
    Set<const CallICFGNode*> indirectCall = edge->getIndirectCalls();
    if (indirectCall.size() > 0)
    {
        indirectCallIds = extractNodesIds(indirectCall);
    }

    std::string directCallIds = "";
    Set<const CallICFGNode*> directCall = edge->getDirectCalls();
    if (directCall.size() > 0)
    {
        directCallIds = extractNodesIds(directCall);
    }

    const std::string queryStatement =
        "MATCH (n:CallGraphNode{id:"+std::to_string(edge->getSrcNode()->getId())+"}), (m:CallGraphNode{id:"+std::to_string(edge->getDstNode()->getId()) + "}) WHERE n.id = " +
        std::to_string(edge->getSrcNode()->getId()) +
        " AND m.id = " + std::to_string(edge->getDstNode()->getId()) +
        " CREATE (n)-[r:CallGraphEdge{csid:" + std::to_string(edge->getCallSiteID()) +
        ", kind:" + std::to_string(edge->getEdgeKind()) +
        ", direct_call_set:'" + directCallIds + "', indirect_call_set:'" +
        indirectCallIds + "'}]->(m)";
    return queryStatement;
}

/// ICFG insertions query statements
const std::string GraphDBClient::intraCFGEdge2DBString(const IntraCFGEdge* edge)
{
    std::string srcKind = getICFGNodeKindString(edge->getSrcNode());
    std::string dstKind = getICFGNodeKindString(edge->getDstNode());
    std::string condition = "";
    if (edge->getCondition() != nullptr)
    {
        condition =
            ", condition_var_id:" + std::to_string(edge->getCondition()->getId()) +
            ", branch_cond_val:" + std::to_string(edge->getSuccessorCondValue());
    }
    else
    {
        condition = ", condition_var_id:-1, branch_cond_val:-1";
    }
    const std::string queryStatement =
        "MATCH (n:" + srcKind + "{id:" + std::to_string(edge->getSrcNode()->getId()) +
        "}), (m:" + dstKind + "{id:" + std::to_string(edge->getDstNode()->getId()) +
        "}) WHERE n.id = " + std::to_string(edge->getSrcNode()->getId()) +
        " AND m.id = " + std::to_string(edge->getDstNode()->getId()) +
        " CREATE (n)-[r:IntraCFGEdge{kind:" + std::to_string(edge->getEdgeKind()) +
        condition + "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::callCFGEdge2DBString(const CallCFGEdge* edge)
{
    std::string srcKind = getICFGNodeKindString(edge->getSrcNode());
    std::string dstKind = getICFGNodeKindString(edge->getDstNode());
    const std::string queryStatement =
        "MATCH (n:" + srcKind + "{id:" + std::to_string(edge->getSrcNode()->getId()) +
        "}), (m:" + dstKind + "{id:" + std::to_string(edge->getDstNode()->getId()) +
        "}) WHERE n.id = " + std::to_string(edge->getSrcNode()->getId()) +
        " AND m.id = " + std::to_string(edge->getDstNode()->getId()) +
        " CREATE (n)-[r:CallCFGEdge{kind:" + std::to_string(edge->getEdgeKind()) +
        ", call_pe_ids:'" + extractEdgesIds(edge->getCallPEs()) + "'}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::retCFGEdge2DBString(const RetCFGEdge* edge)
{
    std::string srcKind = getICFGNodeKindString(edge->getSrcNode());
    std::string dstKind = getICFGNodeKindString(edge->getDstNode());
    std::string ret_pe_id = "";
    if (edge->getRetPE() != nullptr)
    {
        ret_pe_id = ", ret_pe_id:" + std::to_string(edge->getRetPE()->getEdgeID());
    }
    else
    {
        ret_pe_id = ", ret_pe_id:-1";
    }
    const std::string queryStatement =
        "MATCH (n:" + srcKind + "{id:" + std::to_string(edge->getSrcNode()->getId()) +
        "}), (m:" + dstKind + "{id:" + std::to_string(edge->getDstNode()->getId()) +
        "}) WHERE n.id = " + std::to_string(edge->getSrcNode()->getId()) +
        " AND m.id = " + std::to_string(edge->getDstNode()->getId()) +
        " CREATE (n)-[r:RetCFGEdge{kind:" + std::to_string(edge->getEdgeKind()) +
        ret_pe_id + "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::globalICFGNode2DBString(const GlobalICFGNode* node)
{
    const std::string queryStatement ="CREATE (n:GlobalICFGNode {id: " + std::to_string(node->getId()) +
    ", kind: " + std::to_string(node->getNodeKind()) + 
    ", pag_edge_ids:'" + extractEdgesIds(node->getSVFStmts())  +"'"+
    node->sourceLocToDBString() + "})";
    return queryStatement;
}

const std::string GraphDBClient::intraICFGNode2DBString(const IntraICFGNode* node)
{
    const std::string queryStatement ="CREATE (n:IntraICFGNode {id: " + std::to_string(node->getId()) +
    ", kind: " + std::to_string(node->getNodeKind()) +
    ", is_return: " + (node->isRetInst() ? "true" : "false") +
    ", fun_obj_var_id:" + std::to_string(node->getFun()->getId()) +
    ", pag_edge_ids:'" + extractEdgesIds(node->getSVFStmts()) +"'" +
    ", bb_id:" + std::to_string(node->getBB()->getId()) + 
    node->sourceLocToDBString() + "})";
    return queryStatement;
}

const std::string GraphDBClient::interICFGNode2DBString(const InterICFGNode* node)
{
    const std::string queryStatement ="CREATE (n:InterICFGNode {id: " + std::to_string(node->getId()) +
    ", kind: " + std::to_string(node->getNodeKind()) + 
    ", pag_edge_ids:'" + extractEdgesIds(node->getSVFStmts()) +"'"+
    node->sourceLocToDBString() + "})";
    return queryStatement;
}

const std::string GraphDBClient::funEntryICFGNode2DBString(const FunEntryICFGNode* node)
{
    const std::string queryStatement ="CREATE (n:FunEntryICFGNode {id: " + std::to_string(node->getId()) +
    ", kind: " + std::to_string(node->getNodeKind()) +
    ", fun_obj_var_id:" + std::to_string(node->getFun()->getId()) + 
    ", pag_edge_ids:'" + extractEdgesIds(node->getSVFStmts()) +"'" +
    ", bb_id:" + std::to_string(node->getBB()->getId()) +
    ", fp_nodes:'" + extractNodesIds(node->getFormalParms()) +"'"+
    node->sourceLocToDBString() + "})";
    return queryStatement;
}

const std::string GraphDBClient::funExitICFGNode2DBString(const FunExitICFGNode* node)
{
    std::string formalRetId = "";
    if (nullptr == node->getFormalRet())
    {
        formalRetId = ",formal_ret_node_id:-1";
    } else {
        formalRetId = ",formal_ret_node_id:" + std::to_string(node->getFormalRet()->getId());
    }
    const std::string queryStatement ="CREATE (n:FunExitICFGNode {id: " + std::to_string(node->getId()) +
    ", kind: " + std::to_string(node->getNodeKind()) +
    ", fun_obj_var_id:" + std::to_string(node->getFun()->getId()) + 
    ", pag_edge_ids:'" + extractEdgesIds(node->getSVFStmts()) +"'" +
    ", bb_id:" + std::to_string(node->getBB()->getId()) +
    formalRetId + node->sourceLocToDBString() + "})";
    return queryStatement;
}

const std::string GraphDBClient::retICFGNode2DBString(const RetICFGNode* node)
{
    std::string actual_ret_node_id="";
    if (node->getActualRet() != nullptr)
    {
        actual_ret_node_id = ", actual_ret_node_id: " + std::to_string(node->getActualRet()->getId()) ;
    } else {
        actual_ret_node_id = ", actual_ret_node_id: -1";
    }
    const std::string queryStatement ="CREATE (n:RetICFGNode {id: " + std::to_string(node->getId()) +
    ", kind: " + std::to_string(node->getNodeKind()) +
    actual_ret_node_id+
    ", call_block_node_id: " + std::to_string(node->getCallICFGNode()->getId()) +
    ", bb_id: " + std::to_string(node->getBB()->getId()) +
    ", fun_obj_var_id: " + std::to_string(node->getFun()->getId()) +
    ", pag_edge_ids:'" + extractEdgesIds(node->getSVFStmts()) +"'" +
    ", svf_type_id:" + std::to_string(node->getType()->getId()) + node->sourceLocToDBString() + "})";
    return queryStatement;
}

const std::string GraphDBClient::callICFGNode2DBString(const CallICFGNode* node)
{
    std::string fun_name_of_v_call = "";
    std::string vtab_ptr_node_id = "";
    std::string virtual_fun_idx = "";
    std::string is_vir_call_inst = node->isVirtualCall() ? "true" : "false";
    std::string virtualFunAppendix = "";
    int indFunPtrId = -1;
    if (node->isIndirectCall())
    {
        indFunPtrId = node->getIndFunPtr()->getId();
    }
    if (node->isVirtualCall())
    {
        fun_name_of_v_call = ", fun_name_of_v_call: '"+node->getFunNameOfVirtualCall()+"'";
        vtab_ptr_node_id = ", vtab_ptr_node_id:" + std::to_string(node->getVtablePtr()->getId());
        virtual_fun_idx = ", virtual_fun_idx:" + std::to_string(node->getFunIdxInVtable());
        virtualFunAppendix = vtab_ptr_node_id+virtual_fun_idx+fun_name_of_v_call;
    }
    else 
    {
        vtab_ptr_node_id = ", vtab_ptr_node_id:-1";
        virtual_fun_idx = ", virtual_fun_idx:-1";
        virtualFunAppendix = vtab_ptr_node_id+virtual_fun_idx;
    }
    std::string called_fun_obj_var_id = "";
    if (node->getCalledFunction() != nullptr)
    {
        called_fun_obj_var_id = ", called_fun_obj_var_id:" + std::to_string(node->getCalledFunction()->getId());
    }
    else 
    {
        called_fun_obj_var_id = ", called_fun_obj_var_id: -1";
    }
    std::string ret_icfg_node_id = "";
    if (node->getRetICFGNode() != nullptr)
    {
        ret_icfg_node_id = ", ret_icfg_node_id: " + std::to_string(node->getRetICFGNode()->getId());
    }
    else 
    {
        ret_icfg_node_id = ", ret_icfg_node_id: -1";
    }
    const std::string queryStatement ="CREATE (n:CallICFGNode {id: " + std::to_string(node->getId()) +
    ", kind: " + std::to_string(node->getNodeKind()) +
    ret_icfg_node_id +
    ", bb_id: " + std::to_string(node->getBB()->getId()) +
    ", fun_obj_var_id: " + std::to_string(node->getFun()->getId()) +
    ", pag_edge_ids:'" + extractEdgesIds(node->getSVFStmts()) +"'" +
    ", svf_type_id:" + std::to_string(node->getType()->getId()) + 
    ", ap_nodes:'" + extractNodesIds(node->getActualParms()) +"'" +
    called_fun_obj_var_id +
    ", is_vararg: " + (node->isVarArg() ? "true" : "false") +
    ", is_vir_call_inst: " + (node->isVirtualCall() ? "true" : "false") +
    ", ind_fun_ptr_var_id:" + std::to_string(indFunPtrId) +
    virtualFunAppendix+ node->sourceLocToDBString() +"})";
    return queryStatement;
}

/// SVFType insertions query statements
const std::string GraphDBClient::stInfo2DBString(const StInfo* stinfo)
{
    const std::string queryStatement ="CREATE (n:StInfo {st_info_id:" + std::to_string(stinfo->getStinfoId()) +
    ", fld_idx_vec:'" + extractIdxs(stinfo->getFlattenedFieldIdxVec()) +
    "', elem_idx_vec:'" + extractIdxs(stinfo->getFlattenedElemIdxVec()) + 
    "', finfo_types:'" + extractSVFTypes(stinfo->getFlattenFieldTypes()) + 
    "', flatten_element_types:'" + extractSVFTypes(stinfo->getFlattenElementTypes()) + 
    "', fld_idx_2_type_map:'" + extractFldIdx2TypeMap(stinfo->getFldIdx2TypeMap()) +
    "', stride:" + std::to_string(stinfo->getStride()) +
    ", num_of_flatten_elements:" + std::to_string(stinfo->getNumOfFlattenElements()) +
    ", num_of_flatten_fields:" + std::to_string(stinfo->getNumOfFlattenFields()) + "})";
    return queryStatement;
}

const std::string GraphDBClient::SVFPointerType2DBString(const SVFPointerType* type)
{
    std::string is_single_val_ty = type->isSingleValueType() ? "true" : "false";
    const std::string queryStatement ="CREATE (n:SVFPointerType {id:" + std::to_string(type->getId()) +
    ", svf_i8_type_id:" + std::to_string(type->getSVFInt8Type()->getId()) +
    ", svf_ptr_type_id:" + std::to_string(type->getSVFPtrType()->getId()) + 
    ", kind:" + std::to_string(type->getKind()) + 
    ", is_single_val_ty:" + is_single_val_ty + 
    ", byte_size:" + std::to_string(type->getByteSize()) + "})";
    return queryStatement;
}

const std::string GraphDBClient::SVFArrayType2DBString(const SVFArrayType* type)
{
    std::string is_single_val_ty = type->isSingleValueType() ? "true" : "false";
    const std::string queryStatement ="CREATE (n:SVFArrayType {id:" +std::to_string(type->getId()) +
    ", svf_i8_type_id:" + std::to_string(type->getSVFInt8Type()->getId()) +
    ", svf_ptr_type_id:" + std::to_string(type->getSVFPtrType()->getId()) + 
    ", kind:" + std::to_string(type->getKind()) + 
    ", stinfo_node_id:" + std::to_string(type->getTypeInfo()->getStinfoId()) +
    ", is_single_val_ty:" + is_single_val_ty + 
    ", byte_size:" + std::to_string(type->getByteSize()) +
    ", num_of_element:" + std::to_string(type->getNumOfElement()) + 
    ", type_of_element_node_type_id:" + std::to_string(type->getTypeOfElement()->getId()) + "})";
    return queryStatement;
}

const std::string GraphDBClient::SVFIntegerType2DBString(const SVFIntegerType* type)
{
    std::string is_single_val_ty = type->isSingleValueType() ? "true" : "false";
    const std::string queryStatement ="CREATE (n:SVFIntegerType {id:" + std::to_string(type->getId()) +
    ", svf_i8_type_id:" + std::to_string(type->getSVFInt8Type()->getId()) +
    ", svf_ptr_type_id:" + std::to_string(type->getSVFPtrType()->getId()) + 
    ", kind:" + std::to_string(type->getKind()) + 
    ", is_single_val_ty:" + is_single_val_ty + 
    ", byte_size:" + std::to_string(type->getByteSize()) +
    ", single_and_width:" + std::to_string(type->getSignAndWidth()) + "})";
    return queryStatement;
}

const std::string GraphDBClient::SVFFunctionType2DBString(const SVFFunctionType* type)
{
    std::string is_single_val_ty = type->isSingleValueType() ? "true" : "false";
    const std::string queryStatement ="CREATE (n:SVFFunctionType {id:" + std::to_string(type->getId()) +
    ", svf_i8_type_id:" + std::to_string(type->getSVFInt8Type()->getId()) +
    ", svf_ptr_type_id:" + std::to_string(type->getSVFPtrType()->getId()) + 
    ", kind:" + std::to_string(type->getKind()) + 
    ", is_single_val_ty:" + is_single_val_ty + 
    ", byte_size:" + std::to_string(type->getByteSize()) +
    ", params_types_vec:'" + extractSVFTypes(type->getParamTypes()) +
    "', ret_ty_node_id:" + std::to_string(type->getReturnType()->getId()) + "})";
    return queryStatement;
}

const std::string GraphDBClient::SVFStructType2DBString(const SVFStructType* type)
{
    std::string is_single_val_ty = type->isSingleValueType() ? "true" : "false";
    const std::string queryStatement ="CREATE (n:SVFStructType {id:" + std::to_string(type->getId()) +
    ", svf_i8_type_id:" + std::to_string(type->getSVFInt8Type()->getId()) +
    ", svf_ptr_type_id:" + std::to_string(type->getSVFPtrType()->getId()) + 
    ", kind:" + std::to_string(type->getKind()) + 
    ", stinfo_node_id:" + std::to_string(type->getTypeInfo()->getStinfoId()) +
    ", is_single_val_ty:" + is_single_val_ty + 
    ", byte_size:" + std::to_string(type->getByteSize()) +
    ", struct_name:'" + type->getName() + "'" +
    ", fields_id_vec:'" + extractSVFTypes(type->getFieldTypes()) +
    "'})";
    return queryStatement;
}

const std::string GraphDBClient::SVFOtherType2DBString(const SVFOtherType* type)
{
    std::string is_single_val_ty = type->isSingleValueType() ? "true" : "false";
    const std::string queryStatement ="CREATE (n:SVFOtherType {id:" + std::to_string(type->getId()) +
    ", svf_i8_type_id:" + std::to_string(type->getSVFInt8Type()->getId()) +
    ", svf_ptr_type_id:" + std::to_string(type->getSVFPtrType()->getId()) + 
    ", kind:" + std::to_string(type->getKind()) + 
    ", is_single_val_ty:" + is_single_val_ty + 
    ", byte_size:" + std::to_string(type->getByteSize()) +
    ", repr:'" + type->getRepr() + "'})";
    return queryStatement;
}

/// PAG insertions query statements
const std::string GraphDBClient::generateSVFStmtEdgeFieldsStmt(const SVFStmt* stmt)
{
    std::string valueStr = "";
    if (nullptr != stmt->getValue())
    {
        valueStr += ", svf_var_node_id:"+ std::to_string(stmt->getValue()->getId());
    }
    else
    {
        valueStr += ", svf_var_node_id:-1";
    }
    std::string bb_id_str = "";
    if (nullptr != stmt->getBB())
    {
        bb_id_str += ", bb_id:'" + std::to_string(stmt->getBB()->getParent()->getId()) + ":" + std::to_string(stmt->getBB()->getId())+"'";
    }
    else 
    {
        bb_id_str += ", bb_id:''";
    }

    std::string icfg_node_id_str = "";
    if (nullptr != stmt->getICFGNode())
    {
        icfg_node_id_str += ", icfg_node_id:" + std::to_string(stmt->getICFGNode()->getId());
    }
    else 
    {
        icfg_node_id_str += ", icfg_node_id:-1";
    }

    std::string inst2_label_map = "";
    if (nullptr != stmt->getInst2LabelMap() && !stmt->getInst2LabelMap()->empty())
    {
        inst2_label_map += ", inst2_label_map:'"+ extractLabelMap2String(stmt->getInst2LabelMap()) +"'";
    }

    std::string var2_label_map = "";
    if (nullptr != stmt->getVar2LabelMap() && !stmt->getVar2LabelMap()->empty())
    {
        var2_label_map += ", var2_label_map:'"+ extractLabelMap2String(stmt->getVar2LabelMap()) +"'";
    }
    std::string fieldsStr = "";
    fieldsStr += "edge_id: " + std::to_string(stmt->getEdgeID()) + 
    valueStr +
    bb_id_str +
    icfg_node_id_str +
    inst2_label_map +
    var2_label_map +
    ", call_edge_label_counter:" + std::to_string(*(stmt->getCallEdgeLabelCounter())) +
    ", store_edge_label_counter:" + std::to_string(*(stmt->getStoreEdgeLabelCounter())) +
    ", multi_opnd_label_counter:" + std::to_string(*(stmt->getMultiOpndLabelCounter())) +
    ", edge_flag:" + std::to_string(stmt->getEdgeKindWithoutMask());
    return fieldsStr;
}

const std::string GraphDBClient::SVFStmt2DBString(const SVFStmt* stmt)
{
    std::string srcKind = getPAGNodeKindString(stmt->getSrcNode());
    std::string dstKind = getPAGNodeKindString(stmt->getDstNode());
    const std::string queryStatement =
        "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->getSrcNode()->getId())+"}), (m:"+dstKind+"{id:"+std::to_string(stmt->getDstNode()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->getSrcNode()->getId()) +
        " AND m.id = " + std::to_string(stmt->getDstNode()->getId()) +
        " CREATE (n)-[r:SVFStmt{"+
        generateSVFStmtEdgeFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::AssignStmt2DBString(const AssignStmt* stmt)
{
    const std::string srcKind = getPAGNodeKindString(stmt->getRHSVar());
    const std::string dstKind = getPAGNodeKindString(stmt->getLHSVar());
    const std::string queryStatement =
        "MATCH (n:" + srcKind + "{id:" + std::to_string(stmt->getRHSVar()->getId()) +
        "}), (m:" + dstKind + "{id:" + std::to_string(stmt->getLHSVar()->getId()) +
        "}) WHERE n.id = " + std::to_string(stmt->getRHSVar()->getId()) +
        " AND m.id = " + std::to_string(stmt->getLHSVar()->getId()) +
        " CREATE (n)-[r:AssignStmt{" + generateAssignStmtFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) + "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::AddrStmt2DBString(const AddrStmt* stmt)
{
    std::string srcKind = getPAGNodeKindString(stmt->getRHSVar());
    std::string dstKind = getPAGNodeKindString(stmt->getLHSVar());
    const std::string queryStatement =
        "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->getRHSVar()->getId())+"}), (m:"+dstKind+"{id:"+std::to_string(stmt->getLHSVar()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->getRHSVar()->getId()) +
        " AND m.id = " + std::to_string(stmt->getLHSVar()->getId()) +
        " CREATE (n)-[r:AddrStmt{"+
        generateAssignStmtFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        ", arr_size:'" + extractNodesIds(stmt->getArrSize()) +"'"+
        "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::CopyStmt2DBString(const CopyStmt* stmt)
{
    std::string srcKind = getPAGNodeKindString(stmt->getRHSVar());
    std::string dstKind = getPAGNodeKindString(stmt->getLHSVar());
    const std::string queryStatement =
        "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->getRHSVar()->getId())+"}), (m:"+dstKind+"{id:"+std::to_string(stmt->getLHSVar()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->getRHSVar()->getId()) +
        " AND m.id = " + std::to_string(stmt->getLHSVar()->getId()) +
        " CREATE (n)-[r:CopyStmt{"+
        generateAssignStmtFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        ", copy_kind:" + std::to_string(stmt->getCopyKind()) +
        "}]->(m)";
    return queryStatement; 
}

const std::string GraphDBClient::StoreStmt2DBString(const StoreStmt* stmt)
{
    std::string srcKind = getPAGNodeKindString(stmt->getRHSVar());
    std::string dstKind = getPAGNodeKindString(stmt->getLHSVar());
    const std::string queryStatement =
        "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->getRHSVar()->getId())+"}), (m:"+dstKind+"{id:"+std::to_string(stmt->getLHSVar()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->getRHSVar()->getId()) +
        " AND m.id = " + std::to_string(stmt->getLHSVar()->getId()) +
        " CREATE (n)-[r:StoreStmt{"+
        generateAssignStmtFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::LoadStmt2DBString(const LoadStmt* stmt)
{
    std::string srcKind = getPAGNodeKindString(stmt->getRHSVar());
    std::string dstKind = getPAGNodeKindString(stmt->getLHSVar());
    const std::string queryStatement =
        "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->getRHSVar()->getId())+"}), (m:"+dstKind+"{id:"+std::to_string(stmt->getLHSVar()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->getRHSVar()->getId()) +
        " AND m.id = " + std::to_string(stmt->getLHSVar()->getId()) +
        " CREATE (n)-[r:LoadStmt{"+
        generateAssignStmtFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::GepStmt2DBString(const GepStmt* stmt)
{
    std::string srcKind = getPAGNodeKindString(stmt->getRHSVar());
    std::string dstKind = getPAGNodeKindString(stmt->getLHSVar());
    std::ostringstream accessPathStr;
    accessPathStr << "";
    if (!stmt->isVariantFieldGep())
    {
        accessPathStr << ", ap_fld_idx:"
                      << std::to_string(stmt->getConstantStructFldIdx());
    }
    else
    {
        accessPathStr << ", ap_fld_idx:-1";
    }

    if (nullptr != stmt->getAccessPath().gepSrcPointeeType())
    {
        accessPathStr << ", ap_gep_pointee_type_id:"<< stmt->getAccessPath().gepSrcPointeeType()->getId();
    }
    else 
    {
        accessPathStr << ", ap_gep_pointee_type_id:-1";
    }
    if (!stmt->getAccessPath().getIdxOperandPairVec().empty())
    {
        accessPathStr << ", ap_idx_operand_pairs:'"
                      << IdxOperandPairsToString(&(stmt->getAccessPath().getIdxOperandPairVec()))
                      << "'";
    }

    const std::string queryStatement =
        "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->getRHSVar()->getId())+"}), (m:"+dstKind+"{id:"+std::to_string(stmt->getLHSVar()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->getRHSVar()->getId()) +
        " AND m.id = " + std::to_string(stmt->getLHSVar()->getId()) +
        " CREATE (n)-[r:GepStmt{" + generateAssignStmtFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        accessPathStr.str() +
        ", variant_field:" + (stmt->isVariantFieldGep()? "true" : "false") +
        "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::CallPE2DBString(const CallPE* stmt)
{
    std::string callInstStr = "";
    std::string funEntryICFGNodeStr = "";
    if (nullptr != stmt->getCallInst()) 
    {
        callInstStr +=  ", call_icfg_node_id:" + std::to_string(stmt->getCallInst()->getId());
    }
    else
    {
        callInstStr +=  ", call_icfg_node_id:-1";
    }

    if (nullptr != stmt->getFunEntryICFGNode())
    {
        funEntryICFGNodeStr +=  ", fun_entry_icfg_node_id:" + std::to_string(stmt->getFunEntryICFGNode()->getId());
    }
    else 
    {
        funEntryICFGNodeStr +=  ", fun_entry_icfg_node_id:-1";
    }
    std::string srcKind = getPAGNodeKindString(stmt->getRHSVar());
    std::string dstKind = getPAGNodeKindString(stmt->getLHSVar());
    const std::string queryStatement =
        "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->getRHSVar()->getId())+"}), (m:"+dstKind+"{id:"+std::to_string(stmt->getLHSVar()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->getRHSVar()->getId()) +
        " AND m.id = " + std::to_string(stmt->getLHSVar()->getId()) +
        " CREATE (n)-[r:CallPE{"+
        generateAssignStmtFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        callInstStr +
        funEntryICFGNodeStr +
        "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::RetPE2DBString(const RetPE* stmt)
{
    std::string callInstStr = "";
    std::string funExitICFGNodeStr = "";
    if (nullptr != stmt->getCallInst()) 
    {
        callInstStr +=  ", call_icfg_node_id:" + std::to_string(stmt->getCallInst()->getId());
    }
    else 
    {
        callInstStr +=  ", call_icfg_node_id:-1";
    }

    if (nullptr != stmt->getFunExitICFGNode())
    {
        funExitICFGNodeStr +=  ", fun_exit_icfg_node_id:" + std::to_string(stmt->getFunExitICFGNode()->getId());
    }
    else 
    {
        funExitICFGNodeStr +=  ", fun_exit_icfg_node_id:-1";
    }
    std::string srcKind = getPAGNodeKindString(stmt->getRHSVar());
    std::string dstKind = getPAGNodeKindString(stmt->getLHSVar());
    const std::string queryStatement =
        "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->getRHSVar()->getId())+"}), (m:"+dstKind+"{id:"+std::to_string(stmt->getLHSVar()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->getRHSVar()->getId()) +
        " AND m.id = " + std::to_string(stmt->getLHSVar()->getId()) +
        " CREATE (n)-[r:RetPE{"+
        generateAssignStmtFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        callInstStr +
        funExitICFGNodeStr +
        "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::MultiOpndStmt2DBString(const MultiOpndStmt* stmt)
{
    const std::string srcKind = getPAGNodeKindString(stmt->SVFStmt::getSrcNode());
    const std::string dstKind = getPAGNodeKindString(stmt->SVFStmt::getDstNode());
    const std::string queryStatement =
        "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->SVFStmt::getSrcNode()->getId())+"}), (m:"+dstKind+"{id:"+std::to_string(stmt->SVFStmt::getDstNode()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->SVFStmt::getSrcNode()->getId()) +
        " AND m.id = " + std::to_string(stmt->SVFStmt::getDstNode()->getId()) +
        " CREATE (n)-[r:MultiOpndStmt{"+
        generateMultiOpndStmtEdgeFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::PhiStmt2DBString(const PhiStmt* stmt)
{
    const std::string srcKind = getPAGNodeKindString(stmt->SVFStmt::getSrcNode());
    const std::string dstKind = getPAGNodeKindString(stmt->SVFStmt::getDstNode());
    const std::string queryStatement =
        "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->SVFStmt::getSrcNode()->getId())+"}), (m:"+dstKind+"{id:"+std::to_string(stmt->SVFStmt::getDstNode()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->SVFStmt::getSrcNode()->getId()) +
        " AND m.id = " + std::to_string(stmt->SVFStmt::getDstNode()->getId()) +
        " CREATE (n)-[r:PhiStmt{"+
        generateMultiOpndStmtEdgeFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        ", op_icfg_nodes_ids:'" + extractNodesIds(*(stmt->getOpICFGNodeVec())) + "'"+
        "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::SelectStmt2DBString(const SelectStmt* stmt)
{
    const std::string srcKind = getPAGNodeKindString(stmt->SVFStmt::getSrcNode());
    const std::string dstKind = getPAGNodeKindString(stmt->SVFStmt::getDstNode());
    const std::string queryStatement =
       "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->SVFStmt::getSrcNode()->getId())+"}), (m:"+dstKind+"{id:"+std::to_string(stmt->SVFStmt::getDstNode()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->SVFStmt::getSrcNode()->getId()) +
        " AND m.id = " + std::to_string(stmt->SVFStmt::getDstNode()->getId()) +
        " CREATE (n)-[r:SelectStmt{"+
        generateMultiOpndStmtEdgeFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        ", condition_svf_var_node_id:" + std::to_string(stmt->getCondition()->getId()) + 
        "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::CmpStmt2DBString(const CmpStmt* stmt)
{
    const std::string srcKind = getPAGNodeKindString(stmt->SVFStmt::getSrcNode());
    const std::string dstKind = getPAGNodeKindString(stmt->SVFStmt::getDstNode());
    const std::string queryStatement =
        "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->SVFStmt::getSrcNode()->getId())+"}), (m:"+
        dstKind+"{id:"+std::to_string(stmt->SVFStmt::getDstNode()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->SVFStmt::getSrcNode()->getId()) +
        " AND m.id = " + std::to_string(stmt->SVFStmt::getDstNode()->getId()) +
        " CREATE (n)-[r:CmpStmt{"+
        generateMultiOpndStmtEdgeFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        ", predicate:" + std::to_string(stmt->getPredicate()) + 
        "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::BinaryOPStmt2DBString(const BinaryOPStmt* stmt)
{
    const std::string srcKind = getPAGNodeKindString(stmt->SVFStmt::getSrcNode());
    const std::string dstKind = getPAGNodeKindString(stmt->SVFStmt::getDstNode());
    const std::string queryStatement =
       "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->SVFStmt::getSrcNode()->getId())+"}), (m:"+
       dstKind+"{id:"+std::to_string(stmt->SVFStmt::getDstNode()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->SVFStmt::getSrcNode()->getId()) +
        " AND m.id = " + std::to_string(stmt->SVFStmt::getDstNode()->getId()) +
        " CREATE (n)-[r:BinaryOPStmt{"+
        generateMultiOpndStmtEdgeFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        ", op_code:" + std::to_string(stmt->getOpcode()) + 
        "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::UnaryOPStmt2DBString(const UnaryOPStmt* stmt)
{
    const std::string srcKind = getPAGNodeKindString(stmt->SVFStmt::getSrcNode());
    const std::string dstKind = getPAGNodeKindString(stmt->SVFStmt::getDstNode());
    const std::string queryStatement =
        "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->SVFStmt::getSrcNode()->getId())+"}), (m:"+
        dstKind+"{id:"+std::to_string(stmt->SVFStmt::getDstNode()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->SVFStmt::getSrcNode()->getId()) +
        " AND m.id = " + std::to_string(stmt->SVFStmt::getDstNode()->getId()) +
        " CREATE (n)-[r:UnaryOPStmt{"+
        generateSVFStmtEdgeFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        ", op_code:" + std::to_string(stmt->getOpcode()) + 
        "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::BranchStmt2DBString(const BranchStmt* stmt)
{
    const std::string srcKind = getPAGNodeKindString(stmt->SVFStmt::getSrcNode());
    const std::string dstKind = getPAGNodeKindString(stmt->SVFStmt::getDstNode());
    const std::string queryStatement =
        "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->SVFStmt::getSrcNode()->getId())+"}), (m:"+
        dstKind+"{id:"+std::to_string(stmt->SVFStmt::getDstNode()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->SVFStmt::getSrcNode()->getId()) +
        " AND m.id = " + std::to_string(stmt->SVFStmt::getDstNode()->getId()) +
        " CREATE (n)-[r:BranchStmt{"+
        generateSVFStmtEdgeFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        ", successors:'" + extractSuccessorsPairSet2String(&(stmt->getSuccessors())) + "'"+ 
        ", condition_svf_var_node_id:" + std::to_string(stmt->getCondition()->getId()) +
        ", br_inst_svf_var_node_id:" + std::to_string(stmt->getBranchInst()->getId()) +
        "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::TDForkPE2DBString(const TDForkPE* stmt)
{
    std::string callInstStr = "";
    std::string funEntryICFGNodeStr = "";
    if (nullptr != stmt->getCallInst()) 
    {
        callInstStr +=  ", call_icfg_node_id:" + std::to_string(stmt->getCallInst()->getId());
    }
    else 
    {
        callInstStr +=  ", call_icfg_node_id:-1";
    }

    if (nullptr != stmt->getFunEntryICFGNode())
    {
        funEntryICFGNodeStr +=  ", fun_entry_icfg_node_id:" + std::to_string(stmt->getFunEntryICFGNode()->getId());
    }
    else 
    {
        funEntryICFGNodeStr +=  ", fun_entry_icfg_node_id:-1";
    }
    std::string srcKind = getPAGNodeKindString(stmt->getRHSVar());
    std::string dstKind = getPAGNodeKindString(stmt->getLHSVar());
    const std::string queryStatement =
        "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->getRHSVar()->getId())+"}), (m:"+
        dstKind+"{id:"+std::to_string(stmt->getLHSVar()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->getRHSVar()->getId()) +
        " AND m.id = " + std::to_string(stmt->getLHSVar()->getId()) +
        " CREATE (n)-[r:TDForkPE{"+
        generateAssignStmtFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        callInstStr +
        funEntryICFGNodeStr +
        "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::TDJoinPE2DBString(const TDJoinPE* stmt)
{
    std::string callInstStr = "";
    std::string funExitICFGNodeStr = "";
    if (nullptr != stmt->getCallInst()) 
    {
        callInstStr +=  ", call_icfg_node_id:" + std::to_string(stmt->getCallInst()->getId());
    }
    else
    {
        callInstStr +=  ", call_icfg_node_id:-1";
    }

    if (nullptr != stmt->getFunExitICFGNode())
    {
        funExitICFGNodeStr +=  ", fun_exit_icfg_node_id:" + std::to_string(stmt->getFunExitICFGNode()->getId());
    }
    else 
    {
        funExitICFGNodeStr +=  ", fun_exit_icfg_node_id:-1";
    }
    std::string srcKind = getPAGNodeKindString(stmt->getRHSVar());
    std::string dstKind = getPAGNodeKindString(stmt->getLHSVar());
    const std::string queryStatement =
        "MATCH (n:"+srcKind+"{id:"+std::to_string(stmt->getRHSVar()->getId())+"}), (m:"+
        dstKind+"{id:"+std::to_string(stmt->getLHSVar()->getId())+"}) WHERE n.id = " +
        std::to_string(stmt->getRHSVar()->getId()) +
        " AND m.id = " + std::to_string(stmt->getLHSVar()->getId()) +
        " CREATE (n)-[r:TDJoinPE{"+
        generateAssignStmtFieldsStmt(stmt) +
        ", kind:" + std::to_string(stmt->getEdgeKind()) +
        callInstStr +
        funExitICFGNodeStr +
        "}]->(m)";
    return queryStatement;
}

const std::string GraphDBClient::valVar2DBString(const ValVar* var)
{
    const std::string queryStatement ="CREATE (n:ValVar {"+
    getValVarNodeFieldsStmt(var)+
    + ", kind:" + std::to_string(var->getNodeKind())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::objVar2DBString(const ObjVar* var)
{
    const std::string queryStatement ="CREATE (n:ObjVar {"+
    getObjVarNodeFieldsStmt(var)
    + ", kind:" + std::to_string(var->getNodeKind())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::argValVar2DBString(const ArgValVar* var)
{
    const std::string queryStatement ="CREATE (n:ArgValVar {"+
    getValVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + ", cg_node_id:" + std::to_string(var->getParent()->getId())
    + ", arg_no:" + std::to_string(var->getArgNo())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::gepValVar2DBString(const GepValVar* var)
{
    std::ostringstream accessPathFieldsStr;
    accessPathFieldsStr << "";

    if (nullptr != var->getAccessPath().gepSrcPointeeType())
    {
        accessPathFieldsStr << ", ap_gep_pointee_type_id:"<<var->getAccessPath().gepSrcPointeeType()->getId();
    }
    else 
    {
        accessPathFieldsStr << ", ap_gep_pointee_type_id:-1";
    }
    if (!var->getAccessPath().getIdxOperandPairVec().empty())
    {
        accessPathFieldsStr <<", ap_idx_operand_pairs:'"<< IdxOperandPairsToString(&(var->getAccessPath().getIdxOperandPairVec()))<<"'";
    }
    const std::string queryStatement ="CREATE (n:GepValVar {"+
    getValVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + ", base_val_id:" + std::to_string(var->getBaseNode()->getId())
    + ", gep_val_svf_type_id:"+std::to_string(var->getType()->getId()) 
    + ", ap_fld_idx:"+std::to_string(var->getConstantFieldIdx())
    + ", llvm_var_inst_id:" + std::to_string(var->getLLVMVarInstID()) 
    + accessPathFieldsStr.str()
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::baseObjVar2DBString(const BaseObjVar* var)
{
    const std::string queryStatement ="CREATE (n:BaseObjVar {"+
    getBaseObjVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::gepObjVar2DBString(const GepObjVar* var)
{
    const std::string queryStatement ="CREATE (n:BaseObjVar {"+
    getObjVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + ", base_obj_var_node_id:" + std::to_string(var->getBaseObj()->getId())
    + ", app_offset:" + std::to_string(var->getConstantFieldIdx())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::heapObjVar2DBString(const HeapObjVar* var)
{
    const std::string queryStatement ="CREATE (n:HeapObjVar {"+
    getBaseObjVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::stackObjVar2DBString(const StackObjVar* var)
{
    const std::string queryStatement ="CREATE (n:StackObjVar {"+
    getBaseObjVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::funObjVar2DBString(const FunObjVar* var)
{
    std::ostringstream exitBBStr;
    if (var->hasBasicBlock() && nullptr != var->getExitBB())
    {
        exitBBStr << ", exit_bb_id:" << std::to_string(var->getExitBB()->getId());
    } 
    else 
    {
        exitBBStr << ", exit_bb_id:-1";
    }
    std::string annotationsStr = "";
    std::vector<std::string> annotationsVector;
    annotationsVector = ExtAPI::getExtAPI()->getExtFuncAnnotations(var);
    if (annotationsVector.size() > 0)
    {
        annotationsStr = serializeAnnotations(annotationsVector);
    }
    std::ostringstream valNameStr;
    if (var->getName().empty())
    {
        valNameStr << ",val_name:''";
    }
    else 
    {
        valNameStr << ",val_name:'"<<var->getName()<<"'";
    }
    const std::string queryStatement ="CREATE (n:FunObjVar {"+
    getBaseObjVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + ", is_decl:" + (var->isDeclaration()? "true" : "false")
    + ", intrinsic:" + (var->isIntrinsic()? "true" : "false")
    + ", is_addr_taken:" + (var->hasAddressTaken()? "true" : "false")
    + ", is_uncalled:" + (var->isUncalledFunction()? "true" : "false")
    + ", is_not_ret:" + (var->getIsNotRet()? "true" : "false")
    + ", sup_var_arg:" + (var->isVarArg()? "true" : "false")
    + ", fun_type_id:" + std::to_string(var->getFunctionType()->getId())
    + ", real_def_fun_node_id:" + std::to_string(var->getDefFunForMultipleModule()->getId())
    // + ", bb_graph_id:" + std::to_string(node->getBasicBlockGraph()->getFunObjVarId())
    + exitBBStr.str()
    + ", all_args_node_ids:'" + extractNodesIds(var->getArgs()) + "'"
    + ", reachable_bbs:'" + extractNodesIds(var->getReachableBBs()) + "'"
    + ", dt_bbs_map:'" + extractBBsMapWithSet2String(&(var->getDomTreeMap())) + "'"
    + ", pdt_bbs_map:'" + extractBBsMapWithSet2String(&(var->getLoopAndDomInfo()->getPostDomTreeMap())) + "'"
    + ", df_bbs_map:'" + extractBBsMapWithSet2String(&(var->getDomFrontierMap())) + "'"
    + ", bb2_loop_map:'" + extractBBsMapWithSet2String(&(var->getLoopAndDomInfo()->getBB2LoopMap())) + "'"
    + ", bb2_p_dom_level:'" + extractLabelMap2String(&(var->getLoopAndDomInfo()->getBBPDomLevel())) + "'"
    + ", bb2_pi_dom:'" + extractBBsMap2String(&(var->getLoopAndDomInfo()->getBB2PIdom())) + "'"
    + ", func_annotation:'" + annotationsStr + "'"
    + valNameStr.str()
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::funValVar2DBString(const FunValVar* var)
{
    const std::string queryStatement ="CREATE (n:FunValVar {"+
    getValVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + ", fun_obj_var_node_id:" + std::to_string(var->getFunction()->getId())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::globalValVar2DBString(const GlobalValVar* var)
{
    const std::string queryStatement ="CREATE (n:GlobalValVar {"+
    getValVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::constAggValVar2DBString(const ConstAggValVar* var)
{
    const std::string queryStatement ="CREATE (n:ConstAggValVar {"+
    getValVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::constDataValVar2DBString(const ConstDataValVar* var)
{
    const std::string queryStatement ="CREATE (n:ConstDataValVar {"+
    getConstDataValVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::blackHoleValVar2DBString(const BlackHoleValVar* var)
{
    const std::string queryStatement ="CREATE (n:BlackHoleValVar {"+
    getConstDataValVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::constFPValVar2DBString(const ConstFPValVar* var)
{
    const std::string queryStatement ="CREATE (n:ConstFPValVar {"+
    getConstDataValVarNodeFieldsStmt(var) 
    +", kind:" + std::to_string(var->getNodeKind())
    +", dval:"+ std::to_string(var->getFPValue())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::constIntValVar2DBString(const ConstIntValVar* var)
{
    const std::string queryStatement ="CREATE (n:ConstIntValVar {"+
    getConstDataValVarNodeFieldsStmt(var) 
    +", kind:" + std::to_string(var->getNodeKind())
    +", zval:'"+ std::to_string(var->getZExtValue()) + "'"
    +", sval:"+ std::to_string(var->getSExtValue())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::constNullPtrValVar2DBString(const ConstNullPtrValVar* var)
{
    const std::string queryStatement ="CREATE (n:ConstNullPtrValVar {"+
    getConstDataValVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::globalObjVar2DBString(const GlobalObjVar* var)
{
    const std::string queryStatement ="CREATE (n:GlobalObjVar {"+
    getBaseObjVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + ", val_name:'" + var->getName() + "'"
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::constAggObjVar2DBString(const ConstAggObjVar* var)
{
    const std::string queryStatement ="CREATE (n:ConstAggObjVar {"+
    getBaseObjVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::constDataObjVar2DBString(const ConstDataObjVar* var)
{
    const std::string queryStatement ="CREATE (n:ConstDataObjVar {"+
    getConstDataObjVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::constFPObjVar2DBString(const ConstFPObjVar* var)
{
    const std::string queryStatement ="CREATE (n:ConstFPObjVar {"+
    getConstDataObjVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + ", dval:" + std::to_string(var->getFPValue())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::constIntObjVar2DBString(const ConstIntObjVar* var)
{
    const std::string queryStatement ="CREATE (n:ConstIntObjVar {"+
    getConstDataObjVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + ", zval:'" + std::to_string(var->getZExtValue()) + "'"
    + ", sval:" + std::to_string(var->getSExtValue())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::constNullPtrObjVar2DBString(const ConstNullPtrObjVar* var)
{
    const std::string queryStatement ="CREATE (n:ConstNullPtrObjVar {"+
    getConstDataObjVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::retValPN2DBString(const RetValPN* var)
{
    const std::string queryStatement ="CREATE (n:RetValPN {"+
    getValVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    +", call_graph_node_id:"+std::to_string(var->getCallGraphNode()->getId())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::varArgValPN2DBString(const VarArgValPN* var)
{
    const std::string queryStatement ="CREATE (n:VarArgValPN {"+
    getValVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    +", call_graph_node_id:"+std::to_string(var->getFunction()->getId())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::dummyValVar2DBString(const DummyValVar* var)
{
    const std::string queryStatement ="CREATE (n:DummyValVar {"+
    getValVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + "})";
    return queryStatement;
}

const std::string GraphDBClient::dummyObjVar2DBString(const DummyObjVar* var)
{
    const std::string queryStatement ="CREATE (n:DummyObjVar {"+
    getBaseObjVarNodeFieldsStmt(var) 
    + ", kind:" + std::to_string(var->getNodeKind())
    + "})";
    return queryStatement;
}