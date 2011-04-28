/*
Clique Detection Algorithm
Tested and Working as of 4/27/2011
*/

#include <stdio.h>
#include <stdlib.h>
//#include "balance.h"
//COMMON *common from balance.c
//extern int channel_count;
int channel_count = 4;
//int *CL,*cl_b;
//int *malicious;
int malicious[4];
int CL[4][4],cl_b[4][4];
int clcnt = 0,clbcnt = 0,malcnt=0;
int graph[4][4] = {0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0} ; //with 4 cliques - Tested
//int graph[4][4] = {0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0} ; //with 1 clique - Tested
//int graph[4][4] = {0,0,1,1,0,0,1,0,1,1,0,1,1,0,1,0} ; // with 2 cliques - 1011, 0110 -Tested
//int graph[4][4] = {0,1,0,0,1,0,1,1,0,1,0,1,0,1,1,0} ; // with 2 cliques - 1100, 0111 - Tested
//int graph[4][4] = {0,1,1,0,1,0,1,0,1,1,0,1,0,0,1,0} ; // with 2 cliques - 1110, 0011 - Tested
int f[5];
int N[4];


void initialize_array(int a[])
{
    int i;
 for(i = 0;i<channel_count;i++)
    a[i] = 0;

}

void intersection_elements(int N1[], int N2[], int new[])
{
        int i,new_cnt = 0;
        for(i = 0;i < channel_count;i ++)
        {
                if(N1[i] == N2[i]) //found intersection between N1 and N2
                {
                    if(N1[i]!=0)
                    {
                        new[i] = 1;
                    }
                    else
                    {
                        new[i] = 0;
                    }
                }

                else
                        new[i] = 0;
        }
} //Tested


int subtract_element(int P[], int i)
{

    if(P[i] != 0)
    {
        P[i] = 0;
        return 1;
    }
    else
        return 0;


} //Tested

int XUnion_element(int R[], int i)
{
        if(R[i] == 0)
        {
                R[i] = 1;
                printf("\n Unioning!");
                return 1;
        }
        else    return 0;
}


void union_element(int R[], int Rnew[], int i)
{
	int j,ncnt = 0;
	for(j=0;j<channel_count;j++)
	 {
	    if(j == i)
	    {
	        Rnew[ncnt++] = 1;
	    }
        else
        {
            Rnew[ncnt++] = R[j];
        }


	 }
	//Rnew[ncnt]  = i;
	printf("\nUnioning R , i to Rnew");
} //Tested


int is_empty(int P[]){
        int i;
        for(i=0;i<channel_count;i++)
        {
                if(P[i] != 0)
                    return 0;
        }
        return 1;
} //Tested



int is_neighbour(int i,int j){

        if(graph[i][j] != 0 && graph[j][i] != 0){
                return 1;
        }
        return 0;
}


void find_neighbor_set(int curr, int N[])
{
        int i,ncnt = 0;
        for(i=0; i<channel_count; i++)
        {
            if(graph[curr][i] == 1)
            {
                N[i] = 1;
            }
            else
            {
                N[i] = 0;
            }

        }

} // Tested


unsigned char find_node_in_clique(int node_j, int clique[][4])
{
        unsigned char flag = 0;
	int i,j;
        for(i=0;i<clbcnt;i++)
        {
//              for(j=0;j<channel_count;j++)
  //            {
    //                if(node_j == clique[i][j])
      //                  flag = 1;
        //      }
		if(clique[i][node_j] == 1){
			flag =1;
			break;
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
                if(R[i] != 0 ){
                        count++;
                }
        }
return count;
}
/*
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
*/

void print(int a[])
{
    int i=0;
    for(i=0; i<4; i++)
        printf("%d ",a[i]);

    printf("\n");
}

int find_next(int P[])
{
        int i = 0;
        for(i=0; i<channel_count; i++)
        {
                if(P[i] == 1)
                {
                    return i;
                }
        }
        return -1;
}


void findMalicious()
{

	int i,j,cnt = 0;
        for(i=0;i<clcnt;i++)
        {
                //To find those maximal cliques with size > Total Nodes in Graph / 2
		cnt = 0;
                for(j=0;j<channel_count;j++)
                {
                        if(CL[i][j] != 0){
                               cnt ++;
				
			}
			
                }
                if(cnt > channel_count/2) // this is valid clique
                {
			printf("\n\t Current Count: %d for the Clique No: %d",cnt,i);
                       // cl_b[clbcnt][j] = 1; //only this i'th clique will be used later
			for(j = 0; j < channel_count ; j ++ )
				cl_b[clbcnt][j] = CL[i][j];
			clbcnt ++;		

                }
        }



        printf("\n The cliques B  are: (CNT: %d) ... Original Count = %d\n",clbcnt,clcnt);
        for(i=0;i<clbcnt;i++){
                for(j=0;j<channel_count;j++){
//                        if(cl_b[i][j]!=-1)
                                printf("\t %d",cl_b[i][j]);

                }
                printf("\n");
        }



	if(clbcnt != 0) // no cl_b cliques
	{
        for(i = 0; i < channel_count ; i++){
                if(!find_node_in_clique(i,cl_b))
                {
                          //if current node in graph was not found in any of cl_b , it is malicious
                          malicious[malcnt++] = 1;
			printf("\n\t\t\tNode %d is malicious",i);
                }
		else    malicious[malcnt++] = 0;
        }
	}    
	else //cl_b is not empty
	{
		//do nothing
	}

}

void FindConsistencyClique(int R[], int P[], int X[], int curr ){


	int i = 0,j,K;
	int current = curr+1;
	printf("\nNew CALL:  ------------------------------------\n");
	printf("Elements in R : ");
	print(R);
    printf("Elements in P : ");
	print(P);
    printf("Elements in X : ");
	print(X);
	//int *Pnew,*Xnew,*N;
    printf("\n Size of: R: %d ; P: %d ; X: %d ",sizeofR(R),sizeofR(P),sizeofR(X));



	if( is_empty(P) && is_empty(X) && (sizeofR(R)>1) )
	{
		//Report R as Maximal Clique
		int j;
		printf("\nInside New call - Forming Clique -------------------------------------------------Current = %d i = %d\n",current,i);
		printf("\nWeGotClique!!!!!!!!!!!!!!!: ");
		for(j=0 ; j<channel_count ; j++)
		{
			if(R[j]!= 0){
				CL[clcnt][j] = R[j];

			}
			printf("\t %d",CL[clcnt][j]);
		}
        //initialize_array(R);
		clcnt++;


	}
	else
	{
		//Select Pivot Node K
		//K = pivot(P);
		//printf("\n\t\t Got  Pivot: %d",K);
		/*if(K == -1) // error in finding pivot
		{
			printf("\nreturning as K = -1");
			return;
		}*/
		while((i=find_next(P)) != -1)//for(i = current; i<channel_count; i++)
		{
		    printf("\nInside For Loop ------------------------------------Current = %d \n",current);
		    printf("i = %d \n",i);
			//Pnew=(int)malloc(channel_count*sizeof(int));
			//Xnew=(int)malloc(channel_count*sizeof(int));
			//N=(int)malloc(channel_count*sizeof(int));

			//if(K!=P[i] && P[i]!=-1){
			//if(is_neighbour(K,i)==0)
			int Pnew[4],Xnew[4],Rnew[4];
               // printf("\n----------------------------Elements in P : ");
               // print(P);
                initialize_array(Rnew);
                initialize_array(Pnew);
                initialize_array(Xnew);
                //printf("\n----------------------------Elements in P : ");
                //print(P);
				 find_neighbor_set(i,N);




              //  if(sizeofR(P) != 0)
              //  {
				subtract_element(P,i);
				printf("\n\t : After Subtraction of %d from P , size of P is: %d",i,sizeofR(P));
                printf("Elements in P: ");
                print(P);
				//going according to the algorithm... adding Rnew also
				union_element(R,Rnew,i);

				printf("\n\t : After Union of R , size of R is: %d \n Elements in Rnew: ",sizeofR(Rnew));
                print(Rnew);
                printf("\n\n Neighbor Set of: %d \n ",i);
                print(N);
				intersection_elements(P,N,Pnew);
				printf("Pnew: ");
				print(Pnew);
                intersection_elements(X,N,Xnew);
				printf("Xnew: ");
				print(Xnew);
				//copy_array(X)
				printf("\n recursion: Size of: Rnew: %d ; Pnew: %d ; Xnew: %d ; N[%d]:%d",sizeofR(Rnew),sizeofR(Pnew),sizeofR(Xnew),i,sizeofR(N));
				FindConsistencyClique(Rnew, Pnew, Xnew,i);
				union_element(X,X,i);
				printf("\n\t : After UnionofX , size of X is: %d \n After Union of X : ",sizeofR(X));
                print(X);
                //current++;
                //initialize_array(R);
             //   }
		}
		//return;
	}
	printf("\n returning :( ");
	printf("\nAfter The Union -------------------------------------------------Current = %d i = %d\n",current,i);
//	return;
}


//extern int channel_count;
int display(){
	printf("\nInDisplay: No of channels: %d\n",channel_count);
}

int main()
{
	int i=0;
	/*find_clique();
	printf("\n\n Malicious!! X-(: \n ");
//	for(i=0;i<malcnt;i++)
//	{
//		if(malicious[i]!=-1)
//		printf("\t %d;",malicious[i]);
//	}*/
	int N1[4] = {1,0,0,0};
	int N2[4] = {1,0,0,1};
	int N3[4] = {0};
    int R[4] = {0};
    int P[4] = {1,1,1,1};
    int X[4] ={0};
	//intersection_elements(N1, N2, N3);
	//subtract_element(N1,0);
	//XUnion_element(N1,3);
    //union_Relement(N1,N3,0);
    //printf("Empty? : %d\n",is_empty(N1));
    //printf("Size: %d",sizeofR(N3));
    //find_neighbor_set(3,N3);
    FindConsistencyClique(R,P,X,-1);
    initialize_array(malicious);
    findMalicious();
    
    printf("\n\t\tMalicious Array ;:: Finally: \n");
		for(i=0;i<channel_count;i++)
		{
		//if(malicious[i]!=-1)
		printf("\t %d;",malicious[i]);
 
        }
}
