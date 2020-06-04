#pragma once

#include <vector>
#include "model.hh"
#include "data.hh"

using namespace std;

struct NODE {                              // Provides information about a node
	vector <long> houseref;                  // References the list of houses within the node
	long parent;                             // The parent node
	vector <long> child;                     // The child nodes
	vector <long> fine;                      // The child nodes on the fine scale
	long population;                         // The total population in the node
	double sussum;                           // The sum of the relative susceptibility of the population in the node
	double x, y;                             // The position of the node (given by the average of all the houses)
	short done;                              // A flag used to determine if this node has been analysed or not
};

struct LEVEL {                             // Stores information about different levels 
	vector <NODE> node;                      // The nodes at a given level
 	vector <long> donelist;                  // List of nodes which have been processed
	vector <double> add;                     // Used when adding up the tree
};

struct IND {                               // Provides information about an individual
	long noderef;                            // The node on the finescale to which the individual belongs
	long houseref;                           // The house to which the individual belongs
  float sus;                               // The relative susceptibility of an individual
	float inf;                               // The relative infectivity of an individual
	vector <float> X;                        // The design matrix for fixed effects
};

class POPTREE
{
 	public:

	void init(DATA &data, short core);
	void setsus(MODEL &model);                 // Sets the relative susceptibility of individuals
	void setinf(MODEL &model);                 // Sets the relative infectivity of individuals

	vector <LEVEL> lev;
	short level;                               // The number of levels of scale in the model
	vector <vector <long> > subpop;            // List of all individuals in node on the fine scale
	long Cfine;                                // Number of nodes on the fine scale
	vector <IND> ind;
	long areamax;                              // The maximum number of areas 
	long **nMval;                              // These are used to store the matrix M of interactions between individuals
	float ***Mval;
	long ***Mnoderef;
	long **naddnoderef;
	long ***addnoderef;
};
