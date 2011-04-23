#include <stdio.h>
//COMMON *common from balance.c
extern int channel_count;
void find_clique()
{
	int *P,*R,*X,*CL,*cl_b,*malicious;
	P=(int)malloc(channel_count*sizeof(int));
	R=(int)malloc(channel_count*sizeof(int));
	X=(int)malloc(channel_count*sizeof(int));
	cl_b=(int)malloc(channel_count*sizeof(int));
	malicious=(int)malloc(channel_count*sizeof(int));
	CL=(int*)malloc(channel_count*channel_count*sizeof(int));
	
	
	int i,j,pivot,rcnt=0,xcnt=0,clcnt=0,clbcnt=0,malcnt=0;

	//Initialize the all arrays to -1.
	for(i = 0;i < channel_count;i ++)
	{
		P[i]=R[i]=X[i]=-1;
		for(j=0;j<channel_count;j++)
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

int sizeofR(int R[])
{
int i=0, count=0;
        for(i=0; i<channel_count; i++) {
                if(R[i] != -1 ){
                        count++;
                }
        }
return count;
}



void FindConsistencyClique(int R[], int P[], int X[] ){
	

	int i = 0;
	if( is_empty(P) && is_empty(X) && (sizeofR(R)>1) )
	{
		//Report R as Maximal Clique
	}
	else
	{
		//Select Pivot Node K
		for(i=0; i<channel_count; i++)
		{
			if(is_neighbour(K,i))
			{
				subtract_element(P,i);
				union_element(R,i);
				find_common_elements(P,N,new);
				find_common_elements(X,N,'new');
				FindConsistencyClique(R, new, 'new');
				union_elements(X,i);	
			}

		}
	}
}


int is_neighbour(int i,int j){
	
	if(common->graph[i][j] != 0 && common->graph[j][i] != 0){
		return 1;

	}
	return 0;
}


void find_neghbor_set(int curr, int N[])
{
	int i,ncnt = 0;
	for(i = 0; i<channel_count ; i++)
	{
		if(i!=curr)
		{
			if(common->graph[i][curr]!=0 && common->graph[curr][i] != 0)  //putting neighbors of curr in N[]
			{
				N[i] = i;
			}
			else{   
				N[i] = -1;
			}
		}
		else{
			N[i] = -1;
			
		}
	}
}


void find_common_elements(int N1[], int N2[], int new[])
{
	int i,new_cnt = 0;
	for(i = 0;i < channel_count;i ++)
	{
		if(N1[i] == N2[i]) //found intersection between N1 and N2
			new[i] = i;
	}
}


void subtract_element(int P[], int i){

	P[i] = -1;

}

void union_element(int R[], int i){

	if(R[i] != -1){
		R[i] = i;
	}

}

int is_empty(int P[]){
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


//extern int channel_count;
int display(){

printf("\nInDisplay: No of channels: %d\n",channel_count);

}
