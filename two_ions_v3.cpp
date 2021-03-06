
/*
1D-1V Plasma Sheath Code : PICS - (Particle-in-Cell for Sheath) 
*/

/*
Dr. Rakesh Moulick
Lovely Professional University
*/

/*
 
 * Contributor: SAYAN ADHIKARI, CPP-IPR
 *
 
*/

/*
Brief Description: The code solves 1D-1V plasma sheath problem.

The code objectives:

1. Include Two positive ion species and track the resulting dynamics.
2. Check the phase space of the particles 
3. Track a single ion or electron and track its phase space structure.
*/

# include <iostream>
# include <cmath>
# include <cstdlib>
# include <vector>
# include <iterator>        /* Added by SAYAN 12/12/2018  */
# include <list>
# include <ctime>
# include <random>
# include <cstring>
# include <fstream>

using namespace std;

/* Random Number Generator */
std::mt19937 mt_gen(0);
std::uniform_real_distribution<double> rnd_dist(0,1.0);
double rnd()
{
	return rnd_dist(mt_gen);
}

/* FUNCTION - LOADING VARIABLES FROM TEXT FILE (ADDED BY SAYAN on 12/12/2018) */

void getItems(vector<string>& var, string path)
{
    ifstream is(path.c_str());
    istream_iterator<string> start(is), end;
    vector<string> items(start, end);
    var = items;
}


/* Define universal constants */
const double EPS = 8.85418782E-12;    // Vacuum permittivity
const double K = 1.38065E-23;        // Boltzmann Constant
const double ME = 9.10938215E-31;   // electron mass
const double QE = 1.602176565E-19; // Charge of an electron
const double AMU = 1.660538921E-27; 
const double EV_TO_K = 11604.52;
const double pi = 3.14159265359;

/* Define Simulation Parameters*/
const double PLASMA_DEN = 1E16; // Plasma Density
const double DX = 1E-5;         // Cell Spacing 
const double DT = 5E-12;		// Time steps 

const double ELECTRON_TEMP = 1; // electron temperature in eV
const double ION_TEMP_1 = 0.026;  // ion temperature in eV
const double ION_TEMP_2 = 0.026;  // ion temperature in eV

const int NUM_IONS_1 = 20000;      // Number of simulation ions
const int NUM_IONS_2 = 20000;      // Number of simulation ions
const int NUM_ELECTRONS = 50000; // Number of simulation electrons

const int NC =  200;             // Total number of cells
int NUM_TS = 1000;          // Total time steps (default)

/* Class Domain: Hold the domain parameters*/
class Domain
{
public:	
	int ni;      // Number of nodes
	double x0;   // initial position 
	double dx;   // cell spacing
	double xl;   // domain length
	double xmax; // domain maximum position
	
	/* Field Data structures */
	double *phi; // Electric Potential
	double *ef;  // Electric field
	double *rho; // Charge Density 				
};

/* Class Particle: Hold particle position, velocity and particle identity*/
class Particle
{
public:	
	double pos;  // particle position
	double vel; // particle velocity
	int id;  // hold particle identity
	
	// Add a constructor
	Particle(double x, double v):pos(x), vel(v){};
};

/* Class Species: Hold species data*/
class Species
{	
public:
	// Use linked list for the particles
	list<Particle> part_list;	
	double mass;
	double charge;
	double spwt;
	string name;
	
	int NUM; 
	double Temp;
	double *den; 
	double *vel; 
	
	void add(Particle part)
	{
		part.id=part_id++; 
		part_list.push_back(part);
	}	
	
	// Add a constructor
	Species(string name, double mass, double charge, double spwt, int NUM, double Temp)
	{
		setName(name);
		setMass(mass);
		setCharge(charge);
		setSpwt(spwt);
		setNum(NUM);
		setTemp(Temp);
	}	
	
	// Define the constructor functions
	void setName(string name){this->name = name;}
	void setMass(double mass){this->mass = mass;}
	void setCharge(double charge){this->charge = charge;}
	void setSpwt(double spwt){this->spwt = spwt;}
	void setNum (int NUM){this->NUM = NUM;}
	void setTemp(double Temp){this->Temp = Temp;}
	
private:
	int part_id = 0;	
};

// Define Domain and File as the global variable
Domain domain;
FILE *file_res;
FILE *file_ke;
FILE *f1;
FILE *f2;
FILE *f3;
FILE *file_sp;
FILE* gnuplotPipe = popen("/usr/local/bin/gnuplot", "w"); //real time plot using GNUPLOT SAYAN 23/12/2018

// Define Helper functions
void Init(Species *species);
void ScatterSpecies(Species *species); 
void ScatterSpeciesVel(Species *species);
void ComputeRho(Species *ions1, Species *ions2, Species *electrons);
void ComputeEF(double *phi, double *ef);
void PushSpecies(Species *species, double *ef);
void RewindSpecies(Species *species, double *ef);
void Write_ts(int ts, Species *ions1,Species *ions2,Species *electrons);
void Write_Particle(FILE *file, int ts, Species *species);
void WriteKE(double Time, Species *ions1, Species *ions2, Species *electrons);
void Write_Single_Particle(Species *species);
void AddSources(Species *species);

double ComputeKE(Species *species); 
double XtoL(double pos);
double gather(double lc, double *field);
double SampleVel(double T, double mass);

bool SolvePotential(double *phi, double *rho);
bool SolvePotentialDirect(double *phi, double *rho);

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/********************* MAIN FUNCTION ***************************/
int main()
{		
	double Time = 0;
	
	/* FOR LOADING INPUTS ADDED BY SAYAN on 12/12/2018*/
	vector<string> var;
	getItems(var,"input.txt");
    
    
    NUM_TS = std::stoi(var[1]);         /* Modify the final time using input file*/
    
	
	/*Construct the domain parameters*/	
	domain.ni = NC+1;
	domain.dx = DX;
	domain.x0 = 0;
	domain. xl = (domain.ni-1)*domain.dx;
	domain.xmax = domain.x0 + domain.xl;
	
	/*Allocate memory to the domain data structures (Field variables)*/
	domain.phi = new double[domain.ni];
	domain.ef = new double[domain.ni];
	domain.rho = new double[domain.ni];
	
	
	/*Redifine the field variables */
	double *phi = domain.phi;
	double *ef = domain.ef;
	double *rho = domain.rho;
		
	/* Clear the domain fields*/
	
	memset(phi,0,sizeof(double)*domain.ni);
	memset(ef, 0,sizeof(double)*domain.ni);
	memset(rho,0,sizeof(double)*domain.ni);
	
	/**************************************************/
		
	/*Species Info: Create vector to hold the data*/
	vector <Species> species_list;
	
	/* Grab a parameter alpha such that 0<alpha<1.
	This parameter is responsible for the percentage of two ions respectively.
	alpha = 0.3 means 30 percent of that species*/
	
	/* STORING DATA FROM VECTOR TO VARIABLE ADDED BY SAYAN on 12/12/2018*/
	double ALP = std::stod(var[3]);
	double mass_1 = std::stod(var[5])*AMU;
	double mass_2 = std::stod(var[7])*AMU;	

	
	// double ALP = 0.8; // Determines the percentage contribution of First Ion.
	//double mass_1 = 8*AMU; // mass of the heavier particle
	//double mass_2 = 4*AMU; // mass of the lighter particle
	
	/*Calculate the specific weights of the ions and electrons*/
	double spwt_1 = (ALP*PLASMA_DEN*domain.xl)/(NUM_IONS_1);
	double spwt_2 = ((1-ALP)*PLASMA_DEN*domain.xl)/(NUM_IONS_2);
	double electron_spwt = (PLASMA_DEN*domain.xl)/(NUM_ELECTRONS);
	
	/* Add singly charged Ar+ ions and electrons */
	/*********************************************/
	/* Create the species lists*/
	species_list.emplace_back("Heavy-Ion",mass_1,QE,spwt_1, NUM_IONS_1, ION_TEMP_1);
	species_list.emplace_back("Light-Ion",mass_2,QE,spwt_2, NUM_IONS_2, ION_TEMP_2);
	species_list.emplace_back("Electrons",ME,-QE,electron_spwt, NUM_ELECTRONS, ELECTRON_TEMP);
	
	/*Assign the species list as ions and electrons*/
	Species &ions1 = species_list[0];
	Species &ions2 = species_list[1];	
	Species &electrons = species_list[2];
	
	/*Initiate the species density and velocity fields*/
	ions1.den = new double[domain.ni];
	ions2.den = new double[domain.ni];
	electrons.den = new double[domain.ni];
	
	ions1.vel = new double[domain.ni];
	ions2.vel = new double[domain.ni];
	electrons.vel = new double[domain.ni];
	
	/*Initialize electrons and ions */	
	Init(&ions1);
	Init(&ions2);
	Init(&electrons);
	
	for(auto &p:species_list)
		cout<< p.name << '\n' << p.mass<< '\n' << p.charge << '\n' << p.spwt << '\n' << p.NUM << endl <<endl;
	/***************************************************************************/
	
	/*Compute Number Density*/
	ScatterSpecies(&ions1);
	ScatterSpecies(&ions2);
	ScatterSpecies(&electrons);
	
	/*Compute charge density, solve for potential 
	and compute the electric field*/
	
	ComputeRho(&ions1, &ions2, &electrons);
	SolvePotential(phi, rho);
	ComputeEF(phi,ef);
	
	RewindSpecies(&ions1,ef);
	RewindSpecies(&ions2,ef);
	RewindSpecies(&electrons,ef);
	
	/*------------- Print Output ---------------*/
	
	/*create a folder named output and*/ 
	/*delete the previous output folder: print statement is just to show*/
	printf("rm -rf output/*\n");
	system("rm -rf output");
	
	/*create an output folder*/
	system("mkdir output");
	char NAME[50];
	
	file_res = fopen("results.dat","w");	
	file_ke = fopen("ke.dat","w");
	file_sp = fopen("part.dat","w");
	
	
	/*MAIN LOOP*/
	
	for (int ts=0; ts<NUM_TS+1; ts++)
	{				
		//Compute number density
		ScatterSpecies(&ions1);
		ScatterSpecies(&ions2);
		ScatterSpecies(&electrons);
		
		//Compute velocities
		ScatterSpeciesVel(&ions1);
		ScatterSpeciesVel(&ions2);
		ScatterSpeciesVel(&electrons);
		
		//Compute charge density
		ComputeRho(&ions1, &ions2, &electrons);
		
		//SolvePotential(phi, rho);
		SolvePotentialDirect(phi, rho);
		ComputeEF(phi, ef);
		
		//move particles
		PushSpecies(&ions1, ef);
		PushSpecies(&ions2, ef);		
		PushSpecies(&electrons, ef);
		
		//Write diagnostics
		if(ts%50 == 0)
		{			
			sprintf(NAME,"output/i1%d.dat",ts);
			f1 = fopen(NAME,"w");
			
			sprintf(NAME,"output/i2%d.dat",ts);			
			f2 = fopen(NAME,"w");
			
			sprintf(NAME,"output/e%d.dat",ts);
			f3 = fopen(NAME,"w");
			
			double max_phi = phi[0];
			for(int i=0; i<domain.ni; i++)
				if (phi[i]>max_phi) max_phi=phi[i];
			
			//Compute kinetic energy
			//double ke_ions = ComputeKE(&ions)/(ions.NUN*ions.spwt);
			//double ke_electrons = ComputeKE(&electrons)/(electrons.NUN*electrons.spwt);
			
			printf("TS: %i \t delta_phi: %.3g\n", ts, max_phi-phi[0]);
            
		
			WriteKE(Time, &ions1, &ions2, &electrons);				
			Write_ts(ts,&ions1,&ions2,&electrons);	
			
			Write_Particle(f1,ts,&ions1);
			Write_Particle(f2,ts, &ions2);
			Write_Particle(f3,ts, &electrons);

			Write_Single_Particle(&electrons);
            // TEST for live graphics
            fprintf(gnuplotPipe, "plot 'output/i1%d.dat' using 1:2 title 'Ion -1  Phase Space' with dots,'output/i2%d.dat' using 1:2 title 'Ion -2  Phase Space' with dots\n",ts,ts);
            fflush(gnuplotPipe);
		}
						
		Time += DT;		
	}	
	
	/*free up memory*/
	delete phi;
	delete rho;
	delete ef;
		
	return 0;
}

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
/********************* HELPER FUNCTIONS ***************************/

/*Initialize the particle data : initial positions and velocities of each particle*/
void Init(Species *species)
{
	// sample particle positions and velocities
	for(int p=0; p<species->NUM; p++)
	{		
		double x = domain.x0 + rnd()*(domain.ni-1)*domain.dx;
		double v = SampleVel(species->Temp*EV_TO_K, species->mass);
		
		// Add to the list
		species->add(Particle(x,v));
	}
}

/*Module to add further species in each time step: p is a macroparticle*/
/*
void AddSources(Species *species)
{
	for(int p=0; p<7; p++)
	{
		double x = domain.x0 + rnd()*(domain.ni-1)*domain.dx;
		//double x = (domain.xl - domain.x0)/2; 
		//double x = (domain.x0+(domain.ni/100)*domain.dx) + rnd()*domain.xl-((domain.ni/100)*domain.dx);
		double v = SampleVel(species->Temp*EV_TO_K, species->mass);		
		// Add to the list
		species->add(Particle(x,v));
	}
}*/

/*Sample Velocity (According to Birdsall)*/
double SampleVel(double T, double mass)
{
	double v_th = sqrt(2*K*T/mass);
	return v_th*sqrt(2)*(rnd()+rnd()+rnd()-1.5);
}

/*Covert the physical coordinate to the logical coordinate*/
double XtoL(double pos)
{
	double li = (pos-domain.x0)/domain.dx;
	return li;
}

/*scatter the particle data to the mesh and collect the densities at the mesh */
void scatter(double lc, double value, double *field)
{
	int i = (int)lc;
	double di = lc-i;
	field[i] += value*(1-di);
	field[i+1] += value*(di); 
}

/* Gather field values at logical coordinates*/
double gather(double lc, double *field)
{
	int i=(int)lc;
	double di = lc-i;
	double val = field[i]*(1-di) + field[i+1]*(di);
	return val;
}

/*Scatter the particles to the mesh for evaluating densities*/
void ScatterSpecies(Species *species)
{
	/*grab a pointer to the species density data and change 
	the density field using the pointer*/
	
	double *field = species->den;
	
	/*clear the field*/
	memset(field,0,sizeof(double)*domain.ni);
	
	/*scatter particles to the mesh*/
	for(auto &p:species->part_list)
	{
		double lc = XtoL(p.pos);
		scatter(lc,species->spwt,field);
	}
	
	/*divide by cell volume*/
	for(int i=0; i<domain.ni; i++)
		field[i] /=domain.dx;
	
	field[0] *=2.0;
	field[domain.ni-1] *= 2.0;
}

/*Scatter the particles to the mesh for evaluating velocities*/
void ScatterSpeciesVel(Species *species)
{
	/*grab a pointer to the species velocity field and change 
	the velocity field using the pointer*/	
	double *field = species->vel;
	
	/*clear the field*/
	memset(field,0,sizeof(double)*domain.ni);
	
	/*scatter particles to the mesh*/
	for(auto &p:species->part_list)
	{
		double lc = XtoL(p.pos);
		scatter(lc,species->spwt*p.vel,field);
	}
	
	/*divide by cell volume*/
	for(int i=0; i<domain.ni; i++)
		field[i] /=(species->den[i]*domain.dx);
	
	field[0] *=2.0;
	field[domain.ni-1] *= 2.0;
}

//*******************************************************
void PushSpecies(Species *species, double *ef)
{
	// compute charge to mass ratio
	double qm = species->charge/species->mass; 
	list<Particle>::iterator it = species->part_list.begin();
	
	// loop over particles
	while (it!=species->part_list.end())
	{
		// grab a reference to the pointer
		Particle &part = *it; 
		
		// compute particle node position
		double lc = XtoL(part.pos);
		
		// gather electric field onto particle position
		double part_ef = gather(lc,ef);
		
		// advance velocity
		part.vel += DT*qm*part_ef;

		// Advance particle position 
		part.pos += DT*part.vel; 

		// Remove the particles leaving the domain
		if(part.pos < domain.x0 || part.pos >= domain.xmax)
		{
			it = species->part_list.erase(it);
			
			/* Encountering Steady state*/
			//part.pos = (domain.xl - domain.x0)/2; // relocate the particle in the middle of the domain
			//part.pos = domain.x0 + rnd()*(domain.ni - 1)*domain.dx;
			//cout << (domain.x0+(domain.ni/100)*domain.dx) << endl;
			/*
			part.pos = (domain.x0+(domain.ni/100)*domain.dx) + rnd()*domain.xl-((domain.ni/100)*domain.dx);
			part.vel = SampleVel(species->Temp*EV_TO_K, species->mass);
			species->add(Particle(part.pos,part.vel));*/
			
			continue;
		}
		else
			it++;			
	}
}
//*********************************************************
/*Rewind particle velocities by -0.5*DT */
void RewindSpecies(Species *species, double *ef)
{
	// compute charge to mass ratio
	double qm = species->charge/species->mass; 
	for(auto &p:species->part_list)
	{
		// compute particle node position
		double lc = XtoL(p.pos);
		// gather electric field onto the particle position
		double part_ef = gather(lc,ef);
		//advance velocity
		p.vel -= 0.5*DT*qm*part_ef;
	}
}

/* Compute the charge densities */
void ComputeRho(Species *ions1, Species *ions2, Species *electrons)
{
	double *rho = domain.rho;
	memset(rho,0,sizeof(double)*domain.ni);
	
	for(int i=0; i<domain.ni; i++)
		rho[i]=ions1->charge*ions1->den[i] + ions2->charge*ions2->den[i] + electrons->charge*electrons->den[i];
	
	/*Reduce numerical noise by setting the densities to zero when less than 1e8/m^3*/
	if(false){
	for(int i=0; i<domain.ni; i++)
		if(fabs(rho[i])<1e8*QE) rho[i]=0;
	}
}

/* Potential Solver: 1. Gauss-Seidel 2. Direct-Solver*/
bool SolvePotential(double *phi, double *rho)
{
	double L2;
	double dx2 = domain.dx*domain.dx;
	
	// Initialize boundaries
	phi[0]=phi[domain.ni-1]=0;
	
	// Main Solver
	for(int it=0; it<200000; it++)
	{
		for(int i=1; i<domain.ni-1; i++)
		{
			double g = 0.5*(phi[i-1] + phi[i+1] + dx2*rho[i]/EPS);
			phi[i]=phi[i] + 1.4*(g-phi[i]);
		}
		// Check for convergence
		if(it%25==0)
		{
			double sum = 0;
			for(int i=1; i<domain.ni-1; i++)
			{
				double R = -rho[i]/EPS - (phi[i-1]-2*phi[i]+phi[i+1])/dx2;
				sum += R*R;
			}
			L2 = sqrt(sum)/domain.ni;
			if(L2<1e-4){return true;}
			
		}
		//printf("GS-Converged! L2=%g\n",L2);
	}
	printf("Gauss-Siedel solver failed to converge, L2=%g\n",L2);
	return false;
}

/* Potential Direct Solver */

bool SolvePotentialDirect(double *x, double *rho)
{
	/* Set coefficients, precompute them*/
	int ni = domain.ni;
	double dx2 = domain.dx*domain.dx;
	double *a = new double[ni];
	double *b = new double[ni];
	double *c = new double[ni];
	
	/*Centtral difference on internal nodes*/
	for(int i=1; i<ni-1; i++)
	{
		a[i] = 1; b[i] = -2; c[i] = 1;
	}
	
	/*Apply dirichlet boundary conditions on boundaries*/
	a[0]=0; b[0]=1; c[0]=0;
	a[ni-1]=0; b[ni-1]=1; c[ni-1]=0;
	
	/*multiply R.H.S.*/
	for (int i=1; i<ni-1; i++)
		x[i]=-rho[i]*dx2/EPS;
	
	x[0] = 0;
	x[ni-1] = 0;
	
	/*Modify the coefficients*/
	c[0] /=b[0];
	x[0] /=b[0];
	
	for(int i=1; i<ni; i++)
	{
		double id = (b[i]-c[i-1]*a[i]);
		c[i] /= id;
		x[i] = (x[i]-x[i-1]*a[i])/id;
	}
	
	/* Now back substitute */
	for(int i=ni-2; i>=0; i--)
		x[i] = x[i] - c[i]*x[i+1];
	
	return true;
}

/*Compute electric field (differentiating potential)*/
void ComputeEF(double *phi, double *ef)
{
	/*Apply central difference to the inner nodes*/
	for(int i=1; i<domain.ni-1; i++)
		ef[i] = -(phi[i+1]-phi[i-1])/(2*domain.dx);
	
	/*Apply one sided difference at the boundary nodes*/
	ef[0] = -(phi[1]-phi[0])/domain.dx;
	ef[domain.ni-1] = -(phi[domain.ni-1]-phi[domain.ni-2])/domain.dx;
}


/*Write the output with time*/
void Write_ts(int ts, Species *ions1,Species *ions2,Species *electrons)
{		
	for(int i=0; i<domain.ni; i++)
	{		
		fprintf(file_res,"%g \t %g \t %g \t %g \t %g \t %g \t %g \t %g \t %g \t %g\n", i*domain.dx, ions1->den[i], ions2->den[i],
	electrons->den[i], ions1->vel[i], ions2->vel[i], electrons->vel[i], domain.rho[i], domain.phi[i], domain.ef[i]);
			
	}
	//fprintf(file_res,"%g \t %g \t %g\n",ts*DT, gamma_i[domain.ni-1], gamma_e[domain.ni-1]);
	fflush(file_res);
}

/* Write the Output results*/
void Write_Particle(FILE *file, int ts, Species *species)
{	
	for(auto& p: species->part_list)
	{		
		fprintf(file,"%g \t %g\n",p.pos, p.vel);
	} 	
	fflush(file_res);
}

void Write_Single_Particle(Species *species)
{
	list<Particle>::iterator it=species->part_list.begin(); 
	Particle &part = *it;
	for(int i=0; i<10; i++){it++;}
	fprintf(file_sp,"%g \t %g\n",part.pos,part.vel);
	fflush(file_sp);
}

void WriteKE(double Time, Species *ions1, Species *ions2, Species *electrons)
{
	double ke_ions1 = ComputeKE(ions1);
	double ke_ions2 = ComputeKE(ions2);
	double ke_electrons = ComputeKE(electrons);
	
	fprintf(file_ke,"%g \t %g \t %g \t %g\n",Time, ke_ions1, ke_ions2, ke_electrons);
	
	fflush(file_ke);
}

double ComputeKE(Species *species)
{
	double ke = 0;
	for (auto &p:species->part_list)
	{
		ke += p.vel*p.vel;
	}
	/*Multiply 0.5*mass for all particles*/
	ke += 0.5*(species->spwt*species->mass);
	
	/*Convert the kinetic energy in eV units*/
	ke /= QE;
	return ke;
}




 
