// Compile with g++ analysis.cc -o analysis -O3
// To run in simulation mode: ./analysis 1024 0

// The first number gives the number of areas into which the houses are divided (should be a power of 4)
// The second number changes the random seed

// To run in inference mode: ./analysis 1024 1000 0
// Here the second number gives the number of PMCMC samples

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <math.h>
#include <time.h>
#include <sstream>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sys/stat.h>

using namespace std;

#include "types.hh"
#include "functions.hh"
#include "consts.hh"
#include "var.hh"       // Stores all the global variables (Sorry Ian, I know you are shaking your head)
#include "model.hh"
#include "simulate.hh"
#include "PMCMC.hh"

void init();

int main(int argc, char** argv)
{
	switch(argc){
	case 3:   // Simulation mode
		siminf = 1;
		areamax = atoi(argv[1]);   
		srand(atoi(argv[2]));
		break;
		
	case 4:   // Inference mode
		siminf = 0;
		areamax = atoi(argv[1]);   
		nsamp = atoi(argv[2]);               
		srand(atoi(argv[3]));
		break;
		
	default:
		emsg("Wrong number of input parameters\n");
		break;
	}
	
	cout << "Initialising....\n";

	init();	
	
	definemodel();

	cout << "Running....\n";

	timetot = -clock();

	if(siminf == 1) simulatedata();
	else PMCMC();

	timetot += clock();
	
	cout << double(timetot)/CLOCKS_PER_SEC << " Total time\n";
	cout << double(timesim)/CLOCKS_PER_SEC << " Simulation time\n";
	cout << double(timeboot)/CLOCKS_PER_SEC << " Bootstrap time\n";
}

// Returns the number of transitions for individuals going from compartment "from" to compartment "to" 
// in different regions over the time range ti - tf
vector <long> PART::getnumtrans(string from, string to, short ti, short tf)
{
	long d, k, r, tra;
	FEV fe;
	vector <long> num;
	
	tra = 0; while(tra < trans.size() && !(comp[trans[tra].from].name == from && comp[trans[tra].to].name == to)) tra++;
	if(tra == trans.size()) emsg("Finescale: Cannot find transition");
	
	for(r = 0; r < nregion; r++) num.push_back(0);

	for(d = long(fediv*double(ti)/tmax); d <= long(fediv*double(tf)/tmax); d++){
		if(d < fediv){
			for(k = 0; k < fev[d].size(); k++){
				fe = fev[d][k];
				if(fe.t > tf) break;
				if(fe.t > ti && fe.trans == tra) num[ind[fe.ind].region]++;
			}
		}
	}
	
	return num;
}

// Initialises a tree of levels in which the entire population is subdivied onto a finer and finer scale
void init()
{
	long h, l, i, imax, j, jmax, ii, jj, k, kmax, m, mmax, num, c, cmax, cc, ccc, par, s, popu, L, n, flag;
	double x, y, xx, yy, grsi, dd, sum, fac;
	HOUSE ho;
	NODE node;
	
	vector <vector <double> > Mval_sr_temp;
	vector <vector <double> > Mval_lr_temp;
	vector <vector <long> > Mnoderef_temp;
	vector <vector <long> > addnoderef_temp;
	vector <long> housex1, housex2;
	vector <double> grsize;
	vector <long> grL;
	vector< vector< vector <long> > > grid;
	
	for(h = 0; h < nhouse; h++){                                  // Randomly distributes houses
		ho.x = ran();
		ho.y = ran();
		house.push_back(ho);
	}

	for(h = 0; h < nhouse; h++) house[h].ind.push_back(h);        // Randomly distributes individuals into houses
	for(i = nhouse; i < popsize; i++){
		h = long(ran()*nhouse);
		house[h].ind.push_back(i);
	}

																															  // Here a "node" represents a collection of houses
	lev.push_back(LEVEL ());                                      // The first level contains a single node
	for(h = 0; h < nhouse; h++) node.houseref.push_back(h);       // with all the houses
	node.parent = -1;
	lev[0].node.push_back(node);

	l = 0;
	do{
		lev.push_back(LEVEL ());
		
		flag = 0;
		
		 // The next level contains four child nodes 
		 // These are generated by taking the houses in the current node and 
		 // divivding them first horizontally and then vertically to generate four sub groups 
		for(c = 0; c < lev[l].node.size(); c++){  
			sort(lev[l].node[c].houseref.begin(),lev[l].node[c].houseref.end(),compX);
			num = lev[l].node[c].houseref.size();
			
			housex1.clear(); housex2.clear();
			for(j = 0; j < num/2; j++) housex1.push_back(lev[l].node[c].houseref[j]);
			for(j = num/2; j < num; j++) housex2.push_back(lev[l].node[c].houseref[j]);
		
			sort(housex1.begin(),housex1.end(),compY);
			num = housex1.size(); if(num > 2) flag = 1;
			
			node.houseref.clear(); node.child.clear(); 
			for(j = 0; j < num/2; j++) node.houseref.push_back(housex1[j]);

			if(node.houseref.size() > 0){
				node.parent = c;
				lev[l].node[c].child.push_back(lev[l+1].node.size());	
				lev[l+1].node.push_back(node);
			}

			node.houseref.clear(); node.child.clear();
			for(j = num/2; j < num; j++) node.houseref.push_back(housex1[j]);
			if(node.houseref.size() > 0){
				node.parent = c;
				lev[l].node[c].child.push_back(lev[l+1].node.size());	
				lev[l+1].node.push_back(node);
			}
			
			sort(housex2.begin(),housex2.end(),compY);
			num = housex2.size(); if(num > 2) flag = 1;
			
			node.houseref.clear(); node.child.clear(); 
			for(j = 0; j < num/2; j++) node.houseref.push_back(housex2[j]);
			if(node.houseref.size() > 0){
				node.parent = c;
				lev[l].node[c].child.push_back(lev[l+1].node.size());	
				lev[l+1].node.push_back(node);
			}

			node.houseref.clear(); node.child.clear();
			for(j = num/2; j < num; j++) node.houseref.push_back(housex2[j]);
			if(node.houseref.size() > 0){
				node.parent = c;
				lev[l].node[c].child.push_back(lev[l+1].node.size());	
				lev[l+1].node.push_back(node);
			}
		}
		l++;
		if(lev[l-1].node.size() >= areamax) break;
	}while(flag == 1);

	level = l;
	l = level-1;
	Cfine = lev[l].node.size();
	
	cout << level << " Number of levels\n" << Cfine << " Number of regions\n";

	cmax = lev[l].node.size();                    // On each node store all descendent nodes on the fine scale
	for(c = 0; c < cmax; c++) lev[l].node[c].fine.push_back(c);
	
	for(l = level-2; l >= 0; l--){
		cmax = lev[l].node.size();
		for(c = 0; c < cmax; c++){
			jmax = lev[l].node[c].child.size();
			for(j = 0; j < jmax; j++){
				cc = lev[l].node[c].child[j];
				kmax = lev[l+1].node[cc].fine.size();
				for(k = 0; k < kmax; k++) lev[l].node[c].fine.push_back(lev[l+1].node[cc].fine[k]);
			}
		}
	}
	
	l = level-1;
	for(c = 0; c < Cfine; c++){                    // Calculates the population and position of each node
		x = 0; y = 0; popu = 0; 
		imax = lev[l].node[c].houseref.size();
		for(i = 0; i < imax; i++){
			h = lev[l].node[c].houseref[i];
			popu += house[lev[l].node[c].houseref[i]].ind.size();
			x += house[h].x;
			y += house[h].y;
		}
		lev[l].node[c].popu = popu;
		lev[l].node[c].x = x/imax;
		lev[l].node[c].y = y/imax;
		lev[l].node[c].done = 0;
	}
	
	for(l = level-2; l >= 0; l--){
		for(c = 0; c < lev[l].node.size(); c++){
			x = 0; y = 0; popu = 0;
			jmax = lev[l].node[c].child.size();
			for(j = 0; j < jmax; j++){
				cc = lev[l].node[c].child[j];
				popu += lev[l+1].node[cc].popu;
				x += lev[l+1].node[cc].x;
				y += lev[l+1].node[cc].y;
			}
			lev[l].node[c].popu = popu;
			lev[l].node[c].x = x/jmax;
			lev[l].node[c].y = y/jmax;
			lev[l].node[c].done = 0;
		}
	}
	
	grsi = finegridsize;                               // Creates grids that maps the locations of nodes at different levels
	grsize.resize(level); grL.resize(level); grid.resize(level);
	for(l = level-1; l >= 0; l--){
		L = long(1.0/(2*grsi)); if(L < 1) L = 1;
		grsize[l] = grsi;
		grL[l] = L;
		cmax = lev[l].node.size();
		grid[l].resize(L*L);
		for(c = 0; c < cmax; c++){
			i = long(lev[l].node[c].x*L);
			j = long(lev[l].node[c].y*L);	
			grid[l][j*L+i].push_back(c);
		}
		grsi *= 2;
	}
	
	nMval = new long*[Cfine]; Mval = new float**[Cfine]; Mnoderef = new long**[Cfine];
	naddnoderef = new long*[Cfine]; addnoderef = new long**[Cfine]; 
	
	for(c = 0; c < Cfine; c++){                          // Generates the interaction matrix M
		nMval[c] = new long[level]; Mval[c] = new float*[level]; Mnoderef[c] = new long*[level];
		naddnoderef[c] = new long[level]; addnoderef[c] = new long*[level]; 
		
		Mval_sr_temp.clear(); Mval_sr_temp.resize(level);
		Mval_lr_temp.clear(); Mval_lr_temp.resize(level);
		Mnoderef_temp.clear(); Mnoderef_temp.resize(level);
		addnoderef_temp.clear(); addnoderef_temp.resize(level);
		
		if(c%1000 == 0) cout << c << " / "  << Cfine << " Constructing matrix M\n";
		l = level-1;
		
		x = lev[l].node[c].x;
		y = lev[l].node[c].y;
	
		for(l = level-1; l >= 0; l--){
			flag = 0;
			if(l < level-1){
				kmax = lev[l+1].donelist.size();
				for(k = 0; k < kmax; k++){
					cc = lev[l+1].donelist[k];
					par = lev[l+1].node[cc].parent; if(par == -1) emsg("Finescale: EC1");
					if(lev[l].node[par].done == 0){
						lev[l].node[par].done = 1;
						lev[l].donelist.push_back(par);
										
						mmax = lev[l].node[par].child.size();
						for(m = 0; m < mmax; m++){
							ccc = lev[l].node[par].child[m];
							if(lev[l+1].node[ccc].done == 0){
								xx = lev[l+1].node[ccc].x;
								yy = lev[l+1].node[ccc].y;
								dd = ((xx-x)*(xx-x) + (yy-y)*(yy-y))/(d0*d0); if(dd < 0.2) dd = 0.2;
														
								Mnoderef_temp[l+1].push_back(ccc);
								Mval_sr_temp[l+1].push_back(1.0/dd);
								Mval_lr_temp[l+1].push_back(1.0/sqrt(dd));
								flag = 1;
							}
						}
					}
				}
			}

			L = grL[l];	grsi = grsize[l];
			i = long(L*x+0.5)-1;
			j = long(L*x+0.5)-1;

			for(ii = i; ii <= i+1; ii++){
				for(jj = j; jj <= j+1; jj++){
					if(ii >= 0 && ii < L && jj >= 0 && jj < L){
						n = jj*L+ii;
						kmax = grid[l][n].size();
						for(k = 0; k < kmax; k++){
							cc = grid[l][n][k];
							if(lev[l].node[cc].done == 0){
								xx = lev[l].node[cc].x;
								yy = lev[l].node[cc].y;
		
								dd = (xx-x)*(xx-x) + (yy-y)*(yy-y);
								if(dd < grsi*grsi){
									dd /= d0*d0; if(dd < 0.2) dd = 0.2;
					
									Mnoderef_temp[l].push_back(cc);
									Mval_sr_temp[l].push_back(1.0/dd);
									Mval_lr_temp[l].push_back(1.0/sqrt(dd));
									flag = 1;
									
									lev[l].node[cc].done = 1;
									lev[l].donelist.push_back(cc);
								}
							}
						}
					}
				}
			}
		}
	
		sum = 0;                             // Normalises short range M contribution to 0.8 
		for(l = 0; l < level; l++){
			kmax = Mval_sr_temp[l].size(); 
			for(k = 0; k < kmax; k++) sum += Mval_sr_temp[l][k]*lev[l].node[Mnoderef_temp[l][k]].popu;
		}
		
		fac = 0.8/sum;
		for(l = 0; l < level; l++){
			kmax = Mval_sr_temp[l].size(); for(k = 0; k < kmax; k++) Mval_sr_temp[l][k] *= fac;
		}
		
		sum = 0;                              // Normalises long range M contribution to 0.2
		for(l = 0; l < level; l++){
			kmax = Mval_lr_temp[l].size(); 
			for(k = 0; k < kmax; k++) sum += Mval_lr_temp[l][k]*lev[l].node[Mnoderef_temp[l][k]].popu;
		}
		
		fac = 0.2/sum;
		for(l = 0; l < level; l++){
			kmax = Mval_lr_temp[l].size(); for(k = 0; k < kmax; k++) Mval_lr_temp[l][k] *= fac;
		}
		
		for(l = 0; l < level; l++){           // Works out how to add up contributions to the root
			kmax = Mnoderef_temp[l].size(); 
			for(k = 0; k < kmax; k++) lev[l].node[Mnoderef_temp[l][k]].done = 0;
			
			jmax = lev[l].donelist.size(); 
			for(j = 0; j < jmax; j++){
				cc = lev[l].donelist[j];
				if(lev[l].node[cc].done == 1) addnoderef_temp[l].push_back(cc);
			}
		}			
		
		for(l = 0; l < level; l++){           // Stores the results
			kmax = Mval_sr_temp[l].size();
			nMval[c][l] = kmax; Mval[c][l] = new float[kmax];	Mnoderef[c][l] = new long[kmax];
			for(k = 0; k < kmax; k++){
				Mval[c][l][k] = Mval_sr_temp[l][k] + Mval_lr_temp[l][k];
				Mnoderef[c][l][k] = Mnoderef_temp[l][k];
			}
			
			kmax = addnoderef_temp[l].size();
			naddnoderef[c][l] = kmax; addnoderef[c][l] = new long[kmax];
			for(k = 0; k < kmax; k++){
				addnoderef[c][l][k] = addnoderef_temp[l][k];
			}
		}
		
		for(l = 0; l < level; l++){
			kmax = lev[l].donelist.size(); for(k = 0; k < kmax; k++) lev[l].node[lev[l].donelist[k]].done = 0;
			lev[l].donelist.clear();
		}
		
		if(checkon == 1){
			popu = 0;
			for(l = 0; l < level; l++){
				kmax = nMval[c][l]; 
				for(k = 0; k < kmax; k++) popu += lev[l].node[Mnoderef[c][l][k]].popu;
			}
			if(popu != popsize) emsg("Finescale: EC2");
		
			for(l = 0; l < level; l++){
				cmax = lev[l].node.size();
				for(cc = 0; cc < cmax; cc++) if(lev[l].node[cc].done != 0) emsg("Finescale: EC3");
			}
		}
	}	
	
	ind.resize(popsize);	                         // Associates indiividuals with regions. this is done for simulation
	for(c = 0; c < Cfine; c++){                    // purposes, but with the real data these will be health board areas.
		kmax = lev[level-1].node[c].houseref.size();
		for(k = 0; k < kmax; k++){	
			h = lev[level-1].node[c].houseref[k];
			for(j = 0; j < house[h].ind.size(); j++){
				i = house[h].ind[j];
				ind[i].noderef = c;
				ind[i].houseref = h;
				ind[i].region = short(house[h].y*RY)*RX + short(house[h].x*RX);
			}
		}		
	}

	for(l = 0; l < level; l++){
		cmax = lev[l].node.size();
		for(c = 0; c < cmax; c++) lev[l].add.push_back(0); 
	}
	
	subpop.resize(Cfine);                           // Defines the populations of individuals in the finescale nodes 
	for(c = 0; c < Cfine; c++){
		kmax = lev[level-1].node[c].houseref.size();
		for(k = 0; k < kmax; k++){			
			h = lev[level-1].node[c].houseref[k];
			for(i = 0; i < house[h].ind.size(); i++) subpop[c].push_back(house[h].ind[i]);
		}
	}
}
