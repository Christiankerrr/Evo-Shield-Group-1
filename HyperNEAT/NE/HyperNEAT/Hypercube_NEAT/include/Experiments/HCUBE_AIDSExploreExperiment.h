#ifndef HCUBE_AIDSEXPLOREEXPERIMENT_H_INCLUDED
#define HCUBE_AIDSEXPLOREEXPERIMENT_H_INCLUDED

#include "Experiments/HCUBE_Experiment.h"
#include "Experiments/HCUBE_AIDSCommon.h"


namespace HCUBE
{
    class AIDSExploreExperiment : public Experiment
    {
    public:
    protected:
        NEAT::LayeredSubstrate<TYPE> substrates[2];

        boost::shared_ptr<const NEAT::GeneticIndividual> substrateIndividuals[2];

    public:
        AIDSExploreExperiment(string _experimentName, int _threadID);

        virtual ~AIDSExploreExperiment();

        virtual NEAT::GeneticPopulation* createInitialPopulation(int populationSize);

        virtual void populateSubstrate(
            boost::shared_ptr<NEAT::GeneticIndividual> individual,
            int substrateNum = 0
        );

        virtual void processGroup(boost::shared_ptr<NEAT::GeneticGeneration> generation);

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

