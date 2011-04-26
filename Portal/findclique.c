#include <stdio.h>
//#include "balance.h"
//COMMON *common from balance.c
//extern int channel_count;
int channel_count = 5;
//int *CL,*cl_b;
//int *malicious;
int malicious[5];
int CL[10][10],cl_b[10][10];
int clcnt = 0,clbcnt = 0,malcnt=0;
int graph[5][5];
int f[5];



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
        if(R[i] == -1)
        {
                R[i] = i;
		printf("\n Unioning!");
        }
}


void union_Relement(int R[], int i,int Rnew[])
{
	int j,ncnt = 0;
	for(j=0;j<5;j++)
	  if(R[j] != -1)
          {
                Rnew[ncnt++] = R[j];
                
          }
	Rnew[ncnt]  = i;
	printf("\nUnioning R , i to Rnew");
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



int is_neighbour(int i,int j){

        if(graph[i][j] != 0 && graph[j][i] != 0){
                return 1;
        }
        return 0;
}


void find_neighbor_set(int curr, int N[])
{
        int i,ncnt = 0;
        for(i = 0; i<channel_count ; i++)
        {
                if(i!=curr)
                {
                        if(graph[i][curr] != 0 && graph[curr][i] != 0)  //putting neighbors of curr in N[]
                        {
                                N[i] = i;
			//	printf("\n\tHERE");
                        }
                        else{
                                N[i] = -1;
                        }
                }
                else{
                        N[i] = -1;

                }
        }

/*	            printf("\n\n (IN FUNCTION) Neighbor Set of: %d \n ",curr);
                        for(i=0;i<5;i++)
				if(N[i]!=-1)
                                printf("\t %d;",N[i]);*/

}



unsigned char find_node_in_clique(int node_j, int clique[][10])
{
        unsigned char flag = 0;
	int i,j;
        for(i=0;i<clbcnt;i++)
        {
              for(j=0;j<channel_count;j++)
              {
                    if(node_j == clique[i][j])
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

void FindConsistencyClique(int R[], int P[], int X[] );
void find_clique()
{
	int P[5],R[5],X[5];
	int i=0,j=0;
	//P=(int)malloc(channel_count*sizeof(int));
	//R=(int)malloc(channel_count*sizeof(int));
	//X=(int)malloc(channel_count*sizeof(int));
//	cl_b=(int*)malloc(channel_count*sizeof(int));
	//malicious=(int)malloc(channel_count*sizeof(int));
//	CL=(int*)malloc(channel_count*channel_count*sizeof(int));
	
	
        graph[0][0] = 1;
	graph[0][1] = 0;graph[1][0] = 0;
	graph[0][2] = 1;graph[2][0] = 1;
	graph[0][3] = 1;graph[3][0] = 1;
	graph[0][4] = 1;graph[4][0] = 1;

	graph[1][1] = 1;
	graph[1][2] = 1;graph[2][1] = 1;
	graph[1][3] = 1;graph[3][1] = 1;
	graph[1][4] = 1;graph[4][1] = 1;

	graph[2][2] = 1;
	graph[2][3] = 1;graph[3][2] = 1;
	graph[2][4] = 1;graph[4][2] = 1;

	graph[3][3] = 1;
	graph[3][4] = 1;graph[4][3] = 1;
	
	graph[4][4] = 1;
	
	
	
	for(i=0;i<5;i++)
		f[i] = 0;

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

	printf("\n The cliques are: (CNT: %d)\n",clcnt);
	for(i=0;i<clcnt;i++){
		for(j=0;j<5;j++){
			if(CL[i][j]!=-1)
				printf("\t %d",CL[i][j]);
			
		}	
		printf("\n");
	}
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

	

        printf("\n The cliques B  are: (CNT: %d)\n",clcnt);
        for(i=0;i<clbcnt;i++){
                for(j=0;j<5;j++){
                        if(cl_b[i][j]!=-1)
                                printf("\t %d",cl_b[i][j]);

                }
                printf("\n");
        }


	
	
	for(i = 0; i < channel_count ; i++){
		if(!find_node_in_clique(i,cl_b))
		{
			  //if current node in graph was not found in any of cl_b , it is malicious
			  malicious[malcnt++] = i;
		}
		else
		{
			printf("\n %d is in cl_b!!",i);
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


int pivot(int P[])
{
	int i,j;
	for(i = 0;i<5;i++)
	{
		if(f[i] == 0){
			f[i] = 1;
	   		return P[i];
		}
	}
	return -1;
}

void FindConsistencyClique(int R[], int P[], int X[] ){
	

	int i = 0,j,K;
	//int *Pnew,*Xnew,*N;
	int Pnew[5],Xnew[5],N[5],Rnew[5];
  	printf("\n New CALL: Size of: R: %d ; P: %d ; X: %d ",sizeofR(R),sizeofR(P),sizeofR(X));
	if( is_empty(P) && is_empty(X) && (sizeofR(R)>1) )
	{
		//Report R as Maximal Clique
		printf("\nForming Clique");
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
		K = pivot(P);
		printf("\n\t\t Got  Pivot: %d",K);
		if(K == -1) // error in finding pivot
		{
			printf("\nreturning as K = -1");
			return;
		}
		for(i=0; i<channel_count; i++)
		{
			//Pnew=(int)malloc(channel_count*sizeof(int));
			//Xnew=(int)malloc(channel_count*sizeof(int));
			//N=(int)malloc(channel_count*sizeof(int));
		 	for(j = 0;j < channel_count;j ++)
	 	        {
            			Pnew[j]=Xnew[j]=N[j]=Rnew[j]=-1;
        		}
			
			

			if(K!=P[i] && P[i]!=-1){
			if(is_neighbour(K,i)==0)
			{
				 find_neighbor_set(i,N);

			         printf("\n\n Neighbor Set of: %d \n ",i);
                        	 for(j=0;j<5;j++)
                                	if(N[j]!=-1)
                                  		printf("\t %d;",N[j]);

				subtract_element(P,i);
				printf("\n\t : After Subtraction of %d from P , size of P is: %d",i,sizeofR(P));
	
				//going according to the algorithm... adding Rnew also
				union_Relement(R,i,Rnew);

				printf("\n\t : After Union of R , size of R is: %d \t",sizeofR(R));
				for (j =0 ; j<5;j++)
					printf("R[%d]: %d  ",j,R[j]);
				find_common_elements(P,N,Pnew);
				find_common_elements(X,N,Xnew);
				printf("\n recursion: Size of: R: %d ; Pnew: %d ; Xnew: %d ; N[%d]:%d",sizeofR(R),sizeofR(Pnew),sizeofR(Xnew),i,sizeofR(N));
				FindConsistencyClique(Rnew, Pnew, Xnew);
				union_element(X,i);	
				printf("\n\t : After Union of X , size of X is: %d \t",sizeofR(X));
			}
			}

		}
		//return;
	}
	printf("\n returning :( ");
//	return;
}


//extern int channel_count;
int display(){
	printf("\nInDisplay: No of channels: %d\n",channel_count);
}

int main()
{
	int i=0;
	find_clique();
	printf("\n\n Malicious!! X-(: \n ");
	for(i=0;i<malcnt;i++)
	{
		if(malicious[i]!=-1)
		printf("\t %d;",malicious[i]);			
	}
}
