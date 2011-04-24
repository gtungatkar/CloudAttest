#include <stdio.h>
//COMMON *common from balance.c
extern int channel_count;
int *CL,*cl_b;
int clcnt = 0,clbcnt = 0;
void find_clique()
{
	int *P,*R,*X,*malicious;
	P=(int)malloc(channel_count*sizeof(int));
	R=(int)malloc(channel_count*sizeof(int));
	X=(int)malloc(channel_count*sizeof(int));
	cl_b=(int*)malloc(channel_count*sizeof(int));
	malicious=(int)malloc(channel_count*sizeof(int));
	CL=(int*)malloc(channel_count*channel_count*sizeof(int));
	
	
	
	int i,j,malcnt=0;

	//Initialize the all arrays to -1.
	//& Setup the node list P[]
	for(i = 0;i < channel_count;i ++)
	{
		R[i]=X[i]=-1;
		P[i] = i;
		for(j=0;j<channel_count;j++)
			CL[i][j]=-1;
	}

	FindConsistencyClique(R,P,X);
	
	//Now, once we have the cliques, we need to find malicious nodes ...
	//Cliques in CL[][]
	for(i=0;i<clcnt;i++)
	{
		//To find those maximal cliques with size > Total Nodes in Graph / 2
		for(j=0;j<channel_count;j++)
		{
			if(CL[i][j] != -1)
				continue;
		}
		if(j > channel_count/2) // this is valid clique
		{
			cl_b[clbcnt][j] = i; //only this i'th clique will be used later
		}	
		clbcnt++;
	}

	

	
	
	for(i = 0; i < channel_count ; i++){
		if(!find_node_in_clique(P[i],cl_b))
		{
			  //if current node in graph was not found in any of cl_b , it is malicious
			  malicious[malcnt++] = P[i];
		}
	
		//if only 1 maximal clique in cl_b --- ATTACK MODELS FINDING --- NEEDED ????
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


unsigned char find_node_in_clique(int node_j, clique[][])
{
	unsigned char flag = 0;
        for(i=0;i<clbcnt;i++)
        {
              for(j=0;j<channel_count;j++)
              {
                    if(node_j == cl_b[i][j])
                    	flag = 1;
	      }
        }
	if(flag == 0) //node_j is not present in any of the Cliques cl_b
		return 0;
	return 1;
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
		for(i=0 ; i<channel_count ; i++)
		{	
			if(R[i]!= -1)
				CL[clcnt][i] = R[i];
		}
		clcnt++;

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
	return;
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
			if(common->graph[i][curr] != 0 && common->graph[curr][i] != 0)  //putting neighbors of curr in N[]
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


void subtract_element(int P[], int i)
{

	P[i] = -1;

}

void union_element(int R[], int i)
{
	if(R[i] != -1)
	{
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
