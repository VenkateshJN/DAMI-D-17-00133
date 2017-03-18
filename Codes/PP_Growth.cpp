#include<bits/stdc++.h>
#include<fstream>
#include<iostream>
#include<unistd.h>
#include<ios>

#define PB push_back
using namespace std;

/*Defining own classes/data structures */
class TreeNode{
public:
	int item;
	list<TreeNode*> children;	
	TreeNode* parent;	
	vector<int> transList;
	TreeNode() {item=-1;}
	TreeNode(TreeNode* ptr){
		item=-1;
		parent = ptr;
		children.clear(),transList.clear();
	}

	~TreeNode() {
	//	delete parent;
		children.clear(),transList.clear();
	}
};

class PfListEntry : public TreeNode{
public:			
	int freq,ip,idl;
	bool valid;		
	PfListEntry() { idl=0,freq=0,ip=0; valid=false; }
	~PfListEntry() {} 
} ;


class Tree{
public:
	TreeNode* root;
	map<int,PfListEntry> PfList;
	map<int,list<TreeNode *> > itemNodeList;
	vector < pair <int,int> > OneFreqItemsPair;
	map<int,vector<int> > itemTransList;
	map<int,int> OneFreqItemsMap;
	Tree(){
		root= new TreeNode(NULL);		
		PfList.clear();		
		OneFreqItemsPair.clear();		
		OneFreqItemsMap.clear();		
		itemNodeList.clear();
		itemTransList.clear();
	}

	~Tree() { 
		delete root;	
		PfList.clear();
		OneFreqItemsPair.clear();		
		OneFreqItemsMap.clear();		
		itemNodeList.clear();
		itemTransList.clear();
	}
};

/* Global Variables */
double minFreq=0.0,maxIAT=0.0,minPF=0.0,perMax=0,firstTS=0; 
int m=0;
int period=0;
ifstream inputFile1,inputFile2;  // pointer to input dataset file
vector<int> OneFreqItems;
int debug=0,numPatterns=0;
ofstream outputFile;
double countMemory=0,numNodesPfTree=0;
map<string, int>Hash;	
vector<string> ReverseHash;
map<int,int> itemsPFMap;

/* Function Defintions */ 
void outputPfList(Tree*);
void populatePfList(Tree*,string);
void updateMinSupportMaxPeriod();
void sortPfList(Tree*);
void pruneAndSortPfList(Tree*);
int sortFuncDec(pair<int,int>,pair<int,int>);
void outputOneFreqItems(Tree*);
Tree* createPfTree(Tree*,string);
void printTree(TreeNode*);
void minePfPatterns(Tree*);
int sortFuncInc(pair<int,int>,pair<int,int>);
void helperFunc(Tree* , vector<int>);
int satisfyConditions(int,int);
int definitelyPeriodic(int ,int );
int canContinue(int ,int );
void printPfPatterns(vector<int>,int, int);
Tree* createConditionalTree(Tree*, int);
void computePfList(Tree*);
Tree* pruneTailNodes(Tree*,int);
Tree* insertBranch(Tree*,TreeNode*);
int isLinear(TreeNode*);
void genLinear(TreeNode*,vector<int>);
void calculateMemory(Tree*);
void recMemory(TreeNode*);
void process_mem_usage(double &);
string itos(int i);

int main(int argc, char **argv){			
	string fileName;
	fileName = argv[1];
	//cin >> minFreq >> maxIAT;  // in percentage
	period=atoi(argv[2]), minFreq = atof(argv[3]),maxIAT=atof(argv[4]), minPF=atof(argv[5]);
	perMax=atof(argv[6]),firstTS=atof(argv[7]);	
	char outFile[100];
	sprintf(outFile,"../Outputs/patterns_PR_%s_%d_%f_%f_%f.txt",fileName.c_str(),period,minFreq,maxIAT,minPF);
	//cout << fileName << ",";
	//printf("%f,%f,%f,",minFreq,maxIAT,minPF);
	//minFreq = 1,maxIAT=40; // hardcoding
	if(debug) cout << fileName << endl ; 
	
	clock_t initial,middle,final;
	initial=clock();

	Tree *tree = new Tree();

	updateMinSupportMaxPeriod();


	populatePfList(tree,fileName);

	if(debug) outputPfList(tree);
	
	pruneAndSortPfList(tree);
	if(debug) 
	{
		outputPfList(tree);
		cout << endl;
	}
	if(debug) 
	{
		outputOneFreqItems(tree);
		cout << endl;
	}

	
	middle=clock();
	float pfListTime = ( (float)(middle)-(float)(initial) )/CLOCKS_PER_SEC;

	
	tree = createPfTree(tree,fileName);
	if(debug) 
	{
		cout << "\nFinal Tree\n";
		printTree(tree->root);
	}

	OneFreqItems.clear();	
	outputFile.open(outFile);


	minePfPatterns(tree);
	
	final=clock();
	float mineTime=( (float)(final)-(float)(middle) )/CLOCKS_PER_SEC;

	double rss;
    process_mem_usage(rss);
    rss = rss/1024;
	
	calculateMemory(tree);
	countMemory = countMemory / (1024*1024);
	char ch='%';	
	printf("%d,%.2f%c,%.2f%c,%.2f%c,",period,100*(minFreq/m),ch,100*(maxIAT/(m-1)),ch,100*(minPF/(m-1)),ch) ;
	printf("%d,%f,%f,%f\n",numPatterns,pfListTime+mineTime,countMemory,rss);	
	
	return 0;
}

string itos(int i) // convert int to string
{
    stringstream s;
    s << i;
    return s.str();
}

void updateMinSupportMaxPeriod(){
	m=(perMax+period-1)/period;
	minFreq = (minFreq*m*1.0)/100;
	maxIAT = (maxIAT*(m-1)*1.0)/100;
	minPF = (minPF*(m-1)*1.0)/100;
	if(debug) cout << "m: " << m << ", minFreq: " << minFreq << ", maxIAT: " << maxIAT << ", minPF: " << minPF << endl ;
}

void populatePfList(Tree *tree,string fileName)
{
	ReverseHash.clear(); Hash.clear();
	string line,item,xs;	
	inputFile1.open(fileName.c_str());	
	int tid=0,index=0,x=0,ps_cur=0;
	firstTS--;
	while(getline(inputFile1, line))
	{
		istringstream split(line);
		getline(split,item,' ');
		tid=atoi(item.c_str());
		tid=tid-firstTS;

		x=tid%period;
		if(x==0)
			x=period;
		xs=itos(x);
		ps_cur=(tid+period-1)/period;

		for(item; getline(split,item,' ');)
		{				
			
			item=item+":"+xs;
			if(Hash.find(item) == Hash.end() )
			{				
				tree->PfList[index].freq=1;
				/*
				if(tid <= maxIAT)
					tree->PfList[index].ip=1;
				else
					tree->PfList[index].ip=0;				
				*/	
				tree->PfList[index].ip=0;
				tree->PfList[index].idl=ps_cur;

				ReverseHash.push_back(item);
				Hash[item]=index++;
			}								
			else{
				tree->PfList[Hash[item]].freq++;	
				if((ps_cur - tree->PfList[Hash[item]].idl) <= maxIAT)
					tree->PfList[Hash[item]].ip++;				
				tree->PfList[Hash[item]].idl=ps_cur;
			}
			//tree->itemTransList[Hash[item]].push_back(ps_cur);
		}
		/*
		if(debug)
		{
			if(tid%period==0)		
			{
				cout << tid <<endl;
				outputPfList(tree);
			}
		}
		*/
	}
	/*
	// Final Updation of period values	
	for(map <int,PfListEntry>::iterator it=tree->PfList.begin();it!=tree->PfList.end();it++)
	{		
		if ( (perMax-(*it).second.idl) <= maxIAT )
			(*it).second.ip++;
	}
	*/
}	


void outputPfList(Tree *tree)
{
	cout << "In outputPfList\n";	
	for(map <int,PfListEntry>::iterator it=tree->PfList.begin();it!=tree->PfList.end();it++)
		cout << ReverseHash[(*it).first] << " " << (*it).second.freq << " " << (*it).second.ip << endl;	
}

void pruneAndSortPfList(Tree *tree){
	vector <int>	tempV;
	for(map <int,PfListEntry>::iterator it=tree->PfList.begin();it!=tree->PfList.end();it++)
	{
		if ( (*it).second.freq < minFreq || (*it).second.ip < minPF  )
			tempV.PB((*it).first);
		else
		{
		    tree->OneFreqItemsPair.PB(make_pair((*it).first,(*it).second.freq));
   			itemsPFMap[(*it).first] = (*it).second.ip;
  		 	(*it).second.valid = true;
		}
	}
	for(std::vector<int>::iterator it=tempV.begin();it!=tempV.end();it++)
		tree->PfList.erase(*it);

	stable_sort(tree->OneFreqItemsPair.begin(),tree->OneFreqItemsPair.end(),sortFuncDec);
	int n = int (tree->OneFreqItemsPair.size());
	if(debug) cout << "Num of potential periodic items: " << n << endl;	
	for(int i=0;i<n;i++)
		tree->OneFreqItemsMap[tree->OneFreqItemsPair[i].first] = tree->OneFreqItemsPair[i].second;
}
	
void outputOneFreqItems(Tree *tree){	
	cout << "In outputOneFreqItems\n";		
	int n = int (tree->OneFreqItemsPair.size());	
	int item;
	map<int,PfListEntry>::iterator it;
	for(int i=0;i<n;i++){
		item = tree->OneFreqItemsPair[i].first;
		it= tree->PfList.find(item);		
		if( (*it).second.ip >= minPF && (*it).second.freq >= minFreq) 
	   		cout << ReverseHash[item] << " " << tree->OneFreqItemsPair[i].second << " " << itemsPFMap[item] << endl;	
	}
}

int sortFuncDec(pair<int,int> a, pair<int,int> b){	
	if(b.second == a.second)
	{
		if(itemsPFMap[a.first] == itemsPFMap[b.first])
			return a.first < b.first;
		return itemsPFMap[a.first] > itemsPFMap[b.first];		
	}
	return a.second > b.second;
}

Tree* createPfTree(Tree *tree,string fileName){
	string line,item,xs;	
	vector < pair<int,int> > PfList;	
	PfList.clear();	
	list <TreeNode *> tempV;	
	int transID=0,ps_cur,ps_prev=-1,x=0;
	TreeNode* node;
	inputFile2.open(fileName.c_str());		
	while(getline(inputFile2, line))
	{
		istringstream split(line);
		//cout << line << endl;
		getline(split,item,' ');
		transID=atoi(item.c_str());
		transID = transID-firstTS;

		x=transID%period;
		if(x==0)
			x=period;
		xs=itos(x);
		ps_cur=(transID+period-1)/period;

		if(ps_prev==-1)
			ps_prev=ps_cur;

		//cout << transID << " " << ps_cur << " " << ps_prev << endl;

		if(ps_cur-ps_prev>=1 || transID==perMax)
		{

			if(transID==perMax)
			{
				for(item; getline(split,item,' ');)
				{
					item=item+":"+xs;			
					if( (tree->OneFreqItemsMap).find(Hash[item]) != (tree->OneFreqItemsMap).end() )
						PfList.PB(make_pair(Hash[item],tree->OneFreqItemsMap[Hash[item]]));
				}
			}
			stable_sort(PfList.begin(),PfList.end(),sortFuncDec);	

			int index=0,match=0;		
			node = tree->root;
			while(index < int(PfList.size()) ) {
				match=0;
				for( list<TreeNode *>::iterator it_node=(node->children).begin();it_node!=(node->children).end();it_node++){
					if( (*it_node)->item == PfList[index].first )	{
						node = (*it_node);
						match=1;
						index++;
						break;
					}					
				}
				if(match==0)
					break;
			}

			while(index < int(PfList.size()) ) {
				TreeNode *tempNode = new TreeNode(node);
				tempNode->item = PfList[index].first;
				(node->children).PB(tempNode);
				node = tempNode;

				if( (tree->itemNodeList).find(PfList[index].first) == (tree->itemNodeList).end() ){
					tempV.clear();
					tempV.PB(node);
					(tree->itemNodeList)[PfList[index].first] = tempV;
				}
				else{
					(tree->itemNodeList)[PfList[index].first].PB(node);
				}
				index++;
			}
			if( int(PfList.size()) > 0)
				(node->transList).PB(ps_prev);
			PfList.clear();

			/*
			cout << "\nCalling PrintTree func at ps=" << ps_prev << endl;
			printTree(tree->root);
			*/
			ps_prev=ps_cur;
		}
		
		for(item; getline(split,item,' ');)
		{
			item=item+":"+xs;			
			if( (tree->OneFreqItemsMap).find(Hash[item]) != (tree->OneFreqItemsMap).end() )
				PfList.PB(make_pair(Hash[item],tree->OneFreqItemsMap[Hash[item]]));
		}

		//sort(PfList.begin(),PfList.end(),sortFuncDec);		
	}
	tree->OneFreqItemsMap.clear();
	return tree;
}

void printTree(TreeNode *node)
{	
	if(node->item>=0)
		cout << ReverseHash[node->item] << " ";
	if( int(node->transList.size()) >0 )
		cout<<"ps-list: ";
	for (vector<int>::iterator vi=(node->transList).begin();vi!=(node->transList).end();vi++)
		cout<< *vi << " ";
	cout<<endl;
	for (list <TreeNode *>::iterator  it_node=(node->children).begin();it_node!=(node->children).end();it_node++)
		printTree(*it_node);		
	//cout << "NewBranch\n";
}


void minePfPatterns(Tree *tree){
	stable_sort(tree->OneFreqItemsPair.begin(),tree->OneFreqItemsPair.end(),sortFuncDec); // L-order

	if(debug)
		cout << "\nIn minePfPatterns\n";

	for(vector< pair<int,int> >:: reverse_iterator it = tree->OneFreqItemsPair.rbegin();it!=tree->OneFreqItemsPair.rend();it++)
	{
		if(debug)		
			cout << ReverseHash[(*it).first] << " " << (*it).second <<  endl;		
		OneFreqItems.PB( (*it).first);
	}

	vector<int> tempV;  // creating conditional tree for each of the OneFreqItems , starting with lowest support item
	while( int(OneFreqItems.size())>0 ){
		tempV.clear();
		tempV.PB(*(OneFreqItems.begin()));		
		if(debug) cout << "\n-----------------------------\n";
		helperFunc(tree,tempV);				
		OneFreqItems.erase(OneFreqItems.begin());		
	}	
}

int sortFuncInc(pair<int,int> a, pair<int,int> b)
{		
	if(b.second == a.second)
		return a.first < b.first;
	return a.second < b.second;
	
	/*
	if(b.second == a.second)
	{
		if(itemsPFMap[a.first] == itemsPFMap[b.first])
			return a.first > b.first;
		return itemsPFMap[a.first] < itemsPFMap[b.first];		
	}
	return a.second < b.second;	
	*/
}

void helperFunc(Tree *tree, vector<int> itemVec){
	if(debug) cout << "In helperFunc for item " << ReverseHash[itemVec[0]] << endl;
	if(debug) cout << "-----------------------------\n";
	if(debug) cout << "itemVector: ";
	if(debug) for (int i = 0; i < int(itemVec.size()); i++)
		cout << ReverseHash[itemVec[i]] << " ";
	if(debug) cout << endl;
	
	/*
	if( !(satisfyConditions( (tree->PfList)[itemVec[0]].freq, (tree->PfList)[itemVec[0]].ip ) ) ){		
		pruneTailNodes(tree,itemVec[0]);
		return;
	}
	*/

	if(debug) 
	{
		cout << "Printing tree for item " << ReverseHash[itemVec[0]] << endl;
		printTree(tree->root);	
	}


	if(satisfyConditions( (tree->PfList)[itemVec[0]].freq, (tree->PfList)[itemVec[0]].ip))
		printPfPatterns(itemVec,(tree->PfList)[itemVec[0]].freq, (tree->PfList)[itemVec[0]].ip);
	else
	{
		tree=pruneTailNodes(tree,itemVec[0]);
		return;		
	}	

	Tree *conditionalTree = createConditionalTree(tree,itemVec[0]); 

	if ( ((conditionalTree->root)->children).empty() )
		return;

	if(debug) cout << "Printing Full conditional Tree for item " << ReverseHash[itemVec[0]] << endl;
	
	if(debug) printTree(conditionalTree->root);	

	/*
	if ( isLinear(conditionalTree->root) )
	{
		genLinear(conditionalTree->root,itemVec);
		return;
	}*/
	

	vector<int> tempV;
	if(debug) cout << "Executing the last for loop in helperFunc\n";

	for (vector<int>::iterator it=OneFreqItems.begin()+1;it!=OneFreqItems.end();it++)
	{
		if(debug) 	cout << "Next item is " << ReverseHash[*it] << endl;
		if ((conditionalTree->itemNodeList)[*it].empty() ==0){		
		// Calling helpFunc for further extensions of "ij"
		// using tempV for pushing from front side in the itemVec
		tempV.clear();
		tempV=itemVec;
		tempV.insert(tempV.begin(),*it);
		helperFunc(conditionalTree,tempV);
		}
	}	
}

int satisfyConditions(int f, int ip){
	return ( (1.0*f >= minFreq) && (1.0*ip >= minPF) );
}

/*
int definitelyPeriodic(int f, int ip){
	return (f >= minFreq && ip >= minPF);
}
*/

void printPfPatterns(vector<int> item, int sup, int ip){
	if(debug) cout << "In printPfPatterns \n";
	for(std::vector<int>::iterator it=item.begin();it!=item.end();it++)
	{
		outputFile << ReverseHash[*it] << " ";
		if(debug) cout << ReverseHash[*it] << " ";
	}
	outputFile << ", " << sup << " , " << ip;
	if(debug) cout << ", " << sup << " , " << ip;
	if(debug) cout << endl;	
	numPatterns++;
	outputFile << "\n";
}

Tree* createConditionalTree(Tree *tree,int item){
	if(debug) cout << "In createConditionalTree\n";
	Tree* conditionalTree = new Tree();
	list<TreeNode*> itemNodes = tree->itemNodeList[item];
	TreeNode* tempNode;
	for(list<TreeNode*>::iterator it_node=itemNodes.begin();it_node!=itemNodes.end();it_node++){
		tempNode = (*it_node)->parent;
		while(tempNode->parent!=NULL){
			(conditionalTree->itemTransList)[tempNode->item].insert((conditionalTree->itemTransList)[tempNode->item].end(),((*it_node)->transList).begin(),((*it_node)->transList).end());
			(conditionalTree->PfList)[tempNode->item] = (tree->PfList)[tempNode->item];
			tempNode = tempNode->parent;
		}
	}
	
	computePfList(conditionalTree);

	for(list<TreeNode *>::iterator it_node=itemNodes.begin();it_node!=itemNodes.end();it_node++)
		conditionalTree=insertBranch(conditionalTree,*it_node);
		
	if (debug) cout << "Before Pruning\n";
	if (debug) printTree(conditionalTree->root);
	tree=pruneTailNodes(tree,item);
	if (debug) cout << "After Pruning\n";
	if (debug) printTree(conditionalTree->root);
	return conditionalTree;
}

void computePfList(Tree *tree){
	int freq=0,ip=0,ld=0;
	int item,flag=0;
	for(map<int,vector<int> >::iterator it = (tree->itemTransList).begin();it!=(tree->itemTransList).end();it++){
		sort((*it).second.begin(),(*it).second.end());

		freq=0,ip=0,ld=0;
		flag=0;
		for(vector<int>::iterator i=(*it).second.begin();i!=(*it).second.end();i++){
			freq++;
			if(flag==1)
			{
				if( (*i - ld) <= maxIAT)
					ip++;
			}
			flag=1;
			ld=*i;
		}

		/*
		if( (perMax - ld) <= maxIAT)
			ip++;
		*/

		item = (*it).first;
		(tree->PfList)[item].freq=freq;
		(tree->PfList)[item].ip=ip;
		(tree->PfList)[item].idl=ld;
	}

	
	for(map<int,PfListEntry>::iterator it = (tree->PfList).begin();it != (tree->PfList).end();it++){
		if(satisfyConditions((*it).second.freq,(*it).second.ip))		
			(*it).second.valid=true;
		else
			(*it).second.valid=false;
	}

	if(debug) outputPfList(tree);
}

Tree* insertBranch(Tree* conditionalTree,TreeNode* tail)
{
//	if(debug) cout << "in insertBranch \n";
	TreeNode *node=tail->parent;
	stack <TreeNode *> path;		

	// Pushing all the ancestors of item into the stack
//	if(node->parent==NULL)
//		return conditionalTree;
	while (node->parent!=NULL)
	{
//		if(debug) 	cout << node->item << " ";
		if ( (conditionalTree->PfList)[node->item].valid )
			path.push(node);
		node=node->parent;
	}
//	if(debug) 	cout << endl;

	// Converting the above path into tree
	int match=0;
	TreeNode *topNode,*tempNode;
	node=conditionalTree->root;	
	int flag=0;
	while (!path.empty())
	{		
		match=1;
		topNode=path.top();
//		if(debug) cout << topNode->item << " ";
		for (list<TreeNode *>::iterator it_node=(node->children).begin();it_node!=(node->children).end();it_node++)
		{			
			if ( (*it_node)->item==topNode->item )
			{
				if(!flag){			
					flag=1;
//					if(debug)  cout << "\nPartial/Full match found\n";
				}
				match=0;
				path.pop();
				node=*it_node;
				break;
			}
		}
		if(match)
			break;
	}
//	if(debug) cout << "2nd part:\n";
	flag=0;
	while (!path.empty())
	{		
		if(!flag){			
			flag=1;
//			if(debug) cout << "Some Part of the path is inserted for the first time\n";
		}
		topNode=path.top();
		path.pop();

		tempNode = new TreeNode(node);		
		tempNode->item=topNode->item;		
		(node->children).PB(tempNode);
		(conditionalTree->itemNodeList)[tempNode->item].PB(tempNode);		
		node=tempNode;		
	}

	(node->transList).insert((node->transList).end(),(tail->transList).begin(),(tail->transList).end());
	
/*	if(debug)	{
		cout << "Temporary Array contents: ";
		for(vector<int> ::iterator v=node->transList.begin();v!=node->transList.end();v++)
			cout << *v << " ";
		cout << endl;
		cout << "In printTree for one branch\n";
		printTree(conditionalTree->root); 
	} */
		return conditionalTree;
}

Tree* pruneTailNodes(Tree* conditionalTree,int item){	
	if(debug) cout << "In pruneTailNodes for item " << ReverseHash[item] << "\n ";
	TreeNode *parent;
	for (list<TreeNode *>::iterator it_node=(conditionalTree->itemNodeList)[item].begin();it_node!=(conditionalTree->itemNodeList)[item].end();it_node++)
	{
		parent=(*it_node)->parent;
		//if(debug) cout << parent->item;
		(parent->transList).insert((parent->transList).end(),((*it_node)->transList).begin() , ((*it_node)->transList).end() );
	}
	(conditionalTree->itemNodeList)[item].clear();	
	//if(debug) cout << endl;
	return conditionalTree;
}


/*
void genLinear(TreeNode *node, vector<int> preFixNodes)
{
	if(debug)cout <<  "In genLinear\n";
	vector<int> children,tempV;
	children.clear(),tempV.clear();

	int numChildren=0;
	do
	{
		node=(node->children).front();
		children.push_back(node->item);
		numChildren++;
	} while ( !(node->children).empty() );

	long int n=(long int)(pow(2,numChildren))-1;
	if(debug) cout << "numChildren is " << numChildren << " n is " << n << endl;
	while (n>0)
	{		
		tempV=preFixNodes;
		for (int i=0;i<numChildren;i++)	
			if ( (n>>i)&1 )
				tempV.push_back(children[i]);
	
		n--;
		printPfPatterns(tempV);		
	}
	return;
}

int isLinear(TreeNode * node)
{		
	if( (node->children).size()==1 )
		return isLinear((node->children).front());		
	else
		return (node->children).empty();
} */


void calculateMemory(Tree * tree) // of only pf-list and pf-tree
{	
	countMemory=0;
	int debugMemory=0;
	if(debug){
		cout<<sizeof(TreeNode *)<<endl;
		cout<<sizeof(TreeNode )<<endl;		
		cout<<sizeof(Tree *)<<endl;
		cout<<sizeof(Tree )<<endl;
		cout<<sizeof(PfListEntry)<<endl;
	}

	// getting Memory of PFlist
	int x=0;
	for(map <int,PfListEntry>::iterator it=(tree->PfList).begin(); it!=(tree->PfList).end();it++)
	{
		if( (*it).second.valid == true)
		{
			x++;
			countMemory += sizeof( (*it).first ) + 4*sizeof(int);//sizeof(PfListEntry);
		}
	}	
	if(debugMemory)	
		cout << "Number of Items in PF-List: " << x << endl;
	
	
	// getting memory of Pf-Tree
	countMemory = sizeof(TreeNode *); // root
	// getting recursively size of tree
	numNodesPfTree=0;
	recMemory(tree->root);
	if(debugMemory)
		cout << "Number of Nodes in PF-Tree: " << numNodesPfTree << endl;
	return;
}


void recMemory(TreeNode * node)
{
	if (node==NULL)
		return;	
	numNodesPfTree++;
	countMemory += sizeof(node->item) + sizeof(TreeNode *) + sizeof(int)*(node->transList).size();

	for (list <TreeNode *>::iterator it=(node->children).begin();it!=(node->children).end();it++)
		recMemory(*it);
	return ;
}

void process_mem_usage(double& resident_set)
{
   using std::ios_base;
   using std::ifstream;
   using std::string;

   
   resident_set = 0.0;

   // 'file' stat seems to give the most reliable results
   //
   ifstream stat_stream("/proc/self/stat",ios_base::in);

   // dummy vars for leading entries in stat that we don't care about
   //
   string pid, comm, state, ppid, pgrp, session, tty_nr;
   string tpgid, flags, minflt, cminflt, majflt, cmajflt;
   string utime, stime, cutime, cstime, priority, nice;
   string O, itrealvalue, starttime;

   // the two fields we want
   //
   unsigned long vsize;
   long rss;

   stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
               >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
               >> utime >> stime >> cutime >> cstime >> priority >> nice
               >> O >> itrealvalue >> starttime >> vsize >> rss; // don't care about the rest

   stat_stream.close();

   long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
   resident_set = rss * page_size_kb;
}

