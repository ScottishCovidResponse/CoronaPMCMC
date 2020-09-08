// Implements a modified Gillespie algorithm to simulate from the model

#include <iostream>
#include <fstream>
#include <algorithm>

#include <assert.h>
#include <math.h>

#include "simulate.hh"

#include "utils.hh"
#include "timers.hh"
#include "chain.hh"
#include "data.hh"
#include "output.hh"
#include "obsmodel.hh"

using namespace std;

/// Initilaises the simulation
Simulate::Simulate(const Details &details, DATA &data, MODEL &model, const POPTREE &poptree, const Mpi &mpi, Inputs &inputs, Output &output, Obsmodel &obsmodel) : details(details), data(data), model(model), poptree(poptree), mpi(mpi), output(output), obsmodel(obsmodel)
{	
	if(details.mode != inf && mpi.ncore != 1) emsgroot("Simulation only requires one core");

	nsamp = inputs.find_int("nsamp",UNSET);                                             // Sets the number of samples for inference
	if(details.mode == multisim){
		if(nsamp == UNSET) emsgroot("The number of samples must be set");
	}
	
	model.infmax = large;
}

/// Performs a simulation
void Simulate::run()
{
	Chain chain(details,data,model,poptree,obsmodel,0);
	proportions(chain.indevp);
	
	output.simulateddata(chain.trevp,chain.indevp,details.outputdir);
	
	if(1 == 0){ // This is switched on to see distribution for R and eta from the simulated data
		SAMPLE sample; 
		PARAMSAMP paramsamp;		
		vector <SAMPLE> opsamp; 
		vector <PARAMSAMP> psamp;
		
		sample.meas = obsmodel.getmeas(chain.trevp,chain.indevp);
		model.setup(chain.paramval);
		sample.R0 = model.R0calc(chain.paramval);
		sample.phi = model.phi; 
		paramsamp.paramval = chain.paramval;
		opsamp.push_back(sample);
		psamp.push_back(paramsamp);
		
		output.results(psamp,opsamp);
	}
}

/// Runs multiple simulations
void Simulate::multirun()
{		
	vector <SAMPLE> opsamp; 
  vector <PARAMSAMP> psamp;
	
	for(auto s = 0u; s < nsamp; s++){
		cout << "Simulating sample " << (s+1) << endl;
		Chain chain(details,data,model,poptree,obsmodel,0);
		
		SAMPLE sample;
		sample.meas = obsmodel.getmeas(chain.trevp,chain.indevp);
		model.setup(chain.paramval);
		sample.R0 = model.R0calc(chain.paramval);
		sample.phi = model.phi; 
		
		PARAMSAMP paramsamp;		
		paramsamp.paramval = chain.paramval;
		opsamp.push_back(sample);
		psamp.push_back(paramsamp);
	}
	output.results(psamp,opsamp);
}

/// Works out the proportion of individuals which visit different compartments
void Simulate::proportions(const vector< vector <FEV> > &indev)
{
	vector <unsigned int> visit;
	for(auto c = 0u; c < model.comp.size(); c++) visit.push_back(0);
	
	vector <unsigned int> demo;
	for(auto dp = 0u; dp < data.ndemocatpos; dp++) demo.push_back(0);
	
	auto ninf = 0u;
	for(auto i = 0u; i < indev.size(); i++){
		auto emax = indev[i].size();
		if(emax > 0){ 
			ninf++;
			demo[data.ind[i].dp]++;
			 
			auto c = 0u;
			visit[c]++;
			for(auto e = 0u; e < emax; e++){
				if(model.trans[indev[i][e].trans].from != model.trans[indev[i][e].trans].to){
					visit[model.trans[indev[i][e].trans].to]++;
				}
			}
		}
	}
	
	cout << endl << "# Infected individuals: "<< ninf << endl << endl;
	
	cout << "Proportion of infected individuals visiting different compartments:" << endl;
	
	for(auto c = 0u; c < model.comp.size(); c++){
		cout << "  " << model.comp[c].name << ": " << double(100*visit[c])/ninf << "%" << endl;
	}
	cout << endl;
}
