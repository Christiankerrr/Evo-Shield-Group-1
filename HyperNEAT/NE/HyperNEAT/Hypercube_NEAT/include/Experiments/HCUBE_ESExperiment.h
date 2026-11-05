#ifndef HCUBE_ESEXPERIMENT_H_INCLUDED
#define HCUBE_ESEXPERIMENT_H_INCLUDED

#include "Experiments/HCUBE_Experiment.h"
#include "Experiments/HCUBE_AIDSCommon.h"
#include "substrate.h"


namespace HCUBE
{
    class ESExperiment : public Experiment
    {
    public:
    protected:
        // Rebuilt for every individual: ES picks its hidden nodes and links.
        NEAT::FastNetwork<double> substrate;

        // Fixed input/output node positions; hidden nodes are added per individual.
        ES::Substrate layout;

        int numNodesIn, numNodesOut, numScenarios;

        vector<string> dataNames;
        vector<vector<float>> data;
        vector<vector<float>> answers;

    public:
        ESExperiment(string _experimentName, int _threadID);

        virtual ~ESExperiment();

        virtual NEAT::GeneticPopulation* createInitialPopulation(int populationSize);

        virtual void generateSubstrate();

        virtual void populateSubstrate(boost::shared_ptr<NEAT::GeneticIndividual> individual);

        virtual void processGroup(boost::shared_ptr<NEAT::GeneticGeneration> generation);

        virtual double evaluateFitness(vector<vector<float>> outputs);

        virtual void processIndividualPostHoc(boost::shared_ptr<NEAT::GeneticIndividual> individual);

#ifndef HCUBE_NOGUI
        virtual void createIndividualImage(wxDC& drawContext, boost::shared_ptr<NEAT::GeneticIndividual> individual);

        virtual bool handleMousePress(wxMouseEvent& event, wxSize& bitmapSize);
#endif

        virtual inline bool performUserEvaluations()
        {
            return false;// PUT_TRUE_ONLY_IF_DOING_INTERACTIVE_EVOLUTION;
        }

        virtual inline bool isDisplayGenerationResult()
        {
            return false;
        }

        virtual Experiment* clone();

        virtual void resetGenerationData(boost::shared_ptr<NEAT::GeneticGeneration> generation);

        virtual void addGenerationData(boost::shared_ptr<NEAT::GeneticGeneration> generation, boost::shared_ptr<NEAT::GeneticIndividual> individual);
    };

}

#endif

