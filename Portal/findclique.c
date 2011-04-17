
//COMMON *common from balance.c
void find_clique()
{
	int P[20],R[20],X[20],CL[20][20],cl_b[20],malicious[20];
	int i,j,pivot,rcnt=0,xcnt=0,clcnt=0,clbcnt=0,malcnt=0;
	for(i = 0;i < 20;i ++)
	{
		P[i]=R[i]=X[i]=-1;
		for(j=0;j<20;j++)
			CL[i][j]=-1;
	}

	//X contains already processed nodes
	//P contains candidate node list.. in our case.. server's Channelindex
	P[0] = 0;
	P[1] = 2;
	P[2] = 3;
	P[3] = 1;

	//Let us take P[2] as the pivot value
	//Now.. move nodes from P -> R who are not neighbors of P[2] = Channel Index 3
	pivot=2; //it is the index not the value
	i=0;
	while(!empty_P(P)){
		if(i! = 2)
		{
			if(common->graph[P[pivot]][P[i]]==0 && common->graph[P[i]][P[pivot]]==0) //It is P[i] is not the neighbor of P[pivot] .. put in R
			{
				//Moving P[i] form P to R
				R[rcnt++] = P[i];
				P[i] = -1;
			}
		}
		i++;
	}

	//Now inserting nodes from R into X as they have been processed
	for(i=0;i<rcnt;i++)
	{
		X[xcnt++] = R[i]; 
		CL[clcnt][i] = R[i];
		R[i]=-1; //reset R[i] for next iteration
	}//Put each R in a Clique Set CL[][]
	clcnt++;

	//Now start from a different Pivot node and do same as above to get another set R 
	//There should be some recursive procedure for this kind of method.. page 5 RunTest Paper
	//Finally R contains the Maximal Cliques.. each iteration may produce a clique in R
	
	//Now, once we have the cliques, we need to find malicious nodes ...
	for(i=0;i<clcnt;i++)
	{
		//To find those maximal cliques with size > Total Nodes in Graph / 2
		for(j=0;j<20;j++)
		{
			if(CL[i][j] != -1)
				continue;
		}
		if(j > total_nodes_in_graph / 2) // this is valid clique
		{
			cl_b[clbcnt++] = i; //only this i'th clique will be used later
		}
	}


	//first find all the nodes in our common->graph[][]
	for(each node_j in Graph){
		for(i=0;i<clncnt;i++)
		{
			for(j=0;j<20;j++)
			{
				if(node in graph ! = CL[cl_b[i]][j])
					continue;
			}
		}
	
		//if current node in graph was not found in any of cl_b , it is malicious
		malicious[malcnt++] = current_node_j;


		//if only 1 maximal clique in cl_b
			//if number of cliques == 1 , nodes in this clique  => N_i
				//if weights of edges from nodes in N_i to rest of nodes NOT in N_i are all 0s
					//attack model is NCAM??
				//else
					//attack model is NCPM or PTPC
			//if number of cliques >= 2 , nodes in maximal clique are N_i , in rest cliques are N_1i , N_2i ,...
			// Check each clique other than maximum clique
			//if(common->graph[N_i][any_other_clique's_N_ji] == 0)
				//attack model is FTFC
			//else if(common->graph[N_j][any_other_clique's_N_ji] have same weight?)
				//attack model is PTFC
	}



}


int empty_P(int P[]){
int i;
	for(i=0;i<20;i++)
	{
		if(P[i]==-1)
			continue;
		else
			return 0;
	}
	
	return 1;

}
