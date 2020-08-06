// This code generate a hierarchical representation of the houses at different levels of spatial scale

#include <iostream>
#include <algorithm>

#include "math.h"

using namespace std;

#include "utils.hh"
#include "poptree.hh"
#include "consts.hh"
#include "data.hh"

struct POS{
	unsigned int c;
	double dist;
};
bool ordpos(POS lhs, POS rhs) { return lhs.dist < rhs.dist; }
		
/// Initialises a tree of levels in which the entire population is subdivied onto a finer and finer scale
POPTREE::POPTREE(DATA &data)
{
	unsigned int h, l, j, jmax, num, c, cmax, cc, ccc, flag;
	NODE node;
	vector <unsigned int> areax1, areax2;
	
	lev.push_back(LEVEL ());                                       // First level contains a single node with all the areas
	for(h = 0; h < data.narea; h++) node.arearef.push_back(h);      
	node.parent = UNSET;
	lev[0].node.push_back(node);

	l = 0;
	do{
		lev.push_back(LEVEL ());
		
		flag = 0;
		
		 // The next level contains four child nodes 
		 // These are generated by taking the areas in the current node and 
		 // divivding them first horizontally and then vertically to generate four sub groups 
		for(c = 0; c < lev[l].node.size(); c++){  
			data.sortX(lev[l].node[c].arearef);
			num = lev[l].node[c].arearef.size();
			areax1.clear(); areax2.clear();
			for(j = 0; j < num/2; j++) areax1.push_back(lev[l].node[c].arearef[j]);
			for(j = num/2; j < num; j++) areax2.push_back(lev[l].node[c].arearef[j]);
		
			data.sortY(areax1);
			num = areax1.size(); if(num > 2) flag = 1;
			
			node.arearef.clear(); node.child.clear(); 
			for(j = 0; j < num/2; j++) node.arearef.push_back(areax1[j]);

			if(node.arearef.size() > 0){
				node.parent = c;
				lev[l].node[c].child.push_back(lev[l+1].node.size());	
				lev[l+1].node.push_back(node);
			}

			node.arearef.clear(); node.child.clear();
			for(j = num/2; j < num; j++) node.arearef.push_back(areax1[j]);
			if(node.arearef.size() > 0){
				node.parent = c;
				lev[l].node[c].child.push_back(lev[l+1].node.size());	
				lev[l+1].node.push_back(node);
			}
			
			data.sortY(areax2);
			num = areax2.size(); if(num > 2) flag = 1;
			
			node.arearef.clear(); node.child.clear(); 
			for(j = 0; j < num/2; j++) node.arearef.push_back(areax2[j]);
			if(node.arearef.size() > 0){
				node.parent = c;
				lev[l].node[c].child.push_back(lev[l+1].node.size());	
				lev[l+1].node.push_back(node);
			}

			node.arearef.clear(); node.child.clear();
			for(j = num/2; j < num; j++) node.arearef.push_back(areax2[j]);
			if(node.arearef.size() > 0){
				node.parent = c;
				lev[l].node[c].child.push_back(lev[l+1].node.size());	
				lev[l+1].node.push_back(node);
			}
		}
		l++;
	}while(flag == 1);
 
	level = l+1;

	if(lev[l].node.size() != data.narea) emsgEC("Poptree",1);
	
	l = level-2;                                                             // Switches children so the last layer matches order
	cmax = lev[l].node.size();
	for(c = 0; c < cmax; c++){
		jmax = lev[l].node[c].child.size();
		for(j = 0; j < jmax; j++){
			cc = lev[l].node[c].child[j];
			if(lev[level-1].node[cc].arearef.size() != 1) emsgEC("Poptree",2);
			ccc = lev[level-1].node[cc].arearef[0];
			lev[l].node[c].child[j] = ccc;
			lev[level-1].node[ccc].parent = c;
		}		
	}
	for(c = 0; c < data.narea; c++) lev[level-1].node[c].arearef[0] = c;
	
	for(l = 0; l < level; l++){
		cmax = lev[l].node.size();
		for(c = 0; c < cmax; c++) lev[l].add.push_back(0); 
	}
}
