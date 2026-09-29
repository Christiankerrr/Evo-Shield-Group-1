#include "HCUBE_Defines.h"

#include "Experiments/HCUBE_AIDSBalancedExperiment.h"

#define DEBUG_DIRECT_LINKS (0)

#define DEBUG_USE_ABSOLUTE (1)

#define DEBUG_USE_DELTAS (1)

#define DEBUG_ENABLE_BIASES (0)

#define DO_REGULAR_RUN_FOR_POSTHOC (1)

namespace HCUBE
{
    using namespace NEAT;

    AIDSBalancedExperiment::AIDSBalancedExperiment(string _experimentName, int _threadID)
        :
        Experiment(_experimentName, _threadID)
    {
        //MAKE_AN_ENTRY_FOR_EACH_LAYER
            /*
            layerInfo.layerSizes.push_back(Vector2<int>(PUT_LAYER_WIDTH,PUT_LAYER_LENGTH));
            layerInfo.layerIsInput.push_back( PUT_TRUE_ONLY_IF_LAYER_IS_INPUT_LAYER_AND_ACTIVATION_LEVELS_ARE_USER_DEFINED );
            layerInfo.layerLocations.push_back(Vector3<float>(PUT_LAYER_LOCATION_IN_3D_FOR_SUBSTRATE_VISUALIZER));
            layerInfo.layerNames.push_back( GIVE_THE_LAYER_A_UNIQUE_NAME );
            */
            //THIS_IS_AN_EXAMPLE:
        layerInfo.layerSizes.push_back(Vector2<int>(8, 8));
        layerInfo.layerIsInput.push_back(true);
        layerInfo.layerLocations.push_back(Vector3<float>(0, 0, 0));
        layerInfo.layerNames.push_back("Input");

        layerInfo.layerSizes.push_back(Vector2<int>(8, 8));
        layerInfo.layerIsInput.push_back(false);
        layerInfo.layerLocations.push_back(Vector3<float>(0, 4, 0));
        layerInfo.layerNames.push_back("Processing");

        layerInfo.layerSizes.push_back(Vector2<int>(1, 1));
        layerInfo.layerIsInput.push_back(false);
        layerInfo.layerLocations.push_back(Vector3<float>(0, 8, 0));
        layerInfo.layerNames.push_back("Output");


        //ADD_ADJACENCY_INFORMATION_FOR_THE_CREATION_OF_LINKS
            //layerInfo.layerAdjacencyList.push_back(std::pair<string,string>(SOURCE_LAYER, DESTINATION_LAYER));

        //THIS_IS_AN_EXAMPLE:
        layerInfo.layerAdjacencyList.push_back(std::pair<string, string>("Input", "Processing"));
        layerInfo.layerAdjacencyList.push_back(std::pair<string, string>("Processing", "Output"));

        //PUT_TRUE_IF_ALL_INCOMING_LINK_WEIGHTS_ARE_NORMALIZED
        layerInfo.normalize = false;

        //THIS_SHOULD_ONLY_BE_TRUE_FOR_LEGACY_EXPERIMENTS
        layerInfo.useOldOutputNames = false;

        //THIS_CAN_BE_USED_FOR_DISCRETE_SUBSTRATE_EXTRAPOLATION
        layerInfo.layerValidSizes = layerInfo.layerSizes;

        for (int a = 0; a < 2; a++)
        {
            substrates[a].setLayerInfo(layerInfo);
        }
    }

    AIDSBalancedExperiment::~AIDSBalancedExperiment()
    {
    }

    GeneticPopulation* AIDSBalancedExperiment::createInitialPopulation(int populationSize)
    {
        GeneticPopulation* population = new GeneticPopulation();
        vector<GeneticNodeGene> genes;

        genes.push_back(GeneticNodeGene("Bias", "NetworkSensor", 0, false));
        genes.push_back(GeneticNodeGene("X1", "NetworkSensor", 0, false));
        genes.push_back(GeneticNodeGene("Y1", "NetworkSensor", 0, false));
#if DEBUG_USE_ABSOLUTE
        genes.push_back(GeneticNodeGene("X2", "NetworkSensor", 0, false));
        genes.push_back(GeneticNodeGene("Y2", "NetworkSensor", 0, false));
#endif
#if DEBUG_USE_DELTAS
        genes.push_back(GeneticNodeGene("DeltaX", "NetworkSensor", 0, false));
        genes.push_back(GeneticNodeGene("DeltaY", "NetworkSensor", 0, false));
#endif
        genes.push_back(GeneticNodeGene("Output_ab", "NetworkOutputNode", 1, false, ACTIVATION_FUNCTION_SIGMOID));
        genes.push_back(GeneticNodeGene("Output_bc", "NetworkOutputNode", 1, false, ACTIVATION_FUNCTION_SIGMOID));
#if DEBUG_DIRECT_LINKS
        genes.push_back(GeneticNodeGene("Output_ac", "NetworkOutputNode", 1, false, ACTIVATION_FUNCTION_SIGMOID));
#endif
#if DEBUG_ENABLE_BIASES
        genes.push_back(GeneticNodeGene("Bias_b", "NetworkOutputNode", 1, false, ACTIVATION_FUNCTION_SIGMOID));
        genes.push_back(GeneticNodeGene("Bias_c", "NetworkOutputNode", 1, false, ACTIVATION_FUNCTION_SIGMOID));
#endif

        for (int a = 0; a < populationSize; a++)
        {
            boost::shared_ptr<GeneticIndividual> individual(new GeneticIndividual(genes, true, 1.0));

            for (int b = 0; b < 0; b++)
            {
                individual->testMutate();
            }

            population->addIndividual(individual);
        }

        cout << "Finished creating population\n";
        return population;
    }

    void AIDSBalancedExperiment::populateSubstrate(
        boost::shared_ptr<NEAT::GeneticIndividual> individual,
        int substrateNum
    )
    {

        NEAT::LayeredSubstrate<TYPE>* substrate;
        if (substrateNum >= 2)
        {
            throw CREATE_LOCATEDEXCEPTION_INFO("ERROR: INVALID SUBSTRATE INDEX!");
        }

        if (substrateIndividuals[substrateNum] == individual)
        {
            //Don't bother remaking the same substrate
            return;
        }

        substrateIndividuals[substrateNum] = individual;

        substrate = &substrates[substrateNum];

        substrate->populateSubstrate(individual);
    }

    void AIDSBalancedExperiment::processGroup(boost::shared_ptr<NEAT::GeneticGeneration> generation)
    {
        //cout << "Processing evaluation...\n";
        boost::shared_ptr<NEAT::GeneticIndividual> individual = group.front();
        //You get 10 points just for being processed, wahooo!
        individual->setFitness(10);

        populateSubstrate(individual);

        //USE_THE_SUBSTRATE_TO_EVALUATE_THE_INDIVIDUAL_AND_ASSIGN_A_FITNESS_WITH_THE_INDIVIDUAL::REWARD(...)_FUNCTION
    }

    void AIDSBalancedExperiment::processIndividualPostHoc(boost::shared_ptr<NEAT::GeneticIndividual> individual)
    {
#if DO_REGULAR_RUN_FOR_POSTHOC
        //cout << "INDIVIDUAL FITNESS BEFORE: " << individual->getFitness() << endl;
        clearGroup();
        addIndividualToGroup(individual);
        //individual->setUserData(CheckersStats().toString());

        boost::shared_ptr<GeneticGeneration> dummy;
        processGroup(dummy);
#else
        //cout << "INDIVIDUAL FITNESS BEFORE: " << individual->getFitness() << endl;
        clearGroup();
        addIndividualToGroup(individual);
        individual->setUserData(shared_ptr<CheckersStats>(new CheckersStats()));

        THIS_SECTION_WILL_DO_VALIDATION_POST_HOC
            THE_DEFAULT_IS_TO_RUN_100_EVALUATIONS

            for (int a = 0; a < 100; a++)
            {
                shared_ptr<GeneticGeneration> dummy;
                processGroup(dummy);
                //cout << "INDIVIDUAL FITNESS: " << individual->getFitness() << endl;
            }
#endif
    }

#ifndef HCUBE_NOGUI
    void AIDSBalancedExperiment::createIndividualImage(wxDC& drawContext, boost::shared_ptr<NEAT::GeneticIndividual> individual)
    {
        if (lastIndividualSeen != individual)
        {
            //cout << "Repopulating substrate\n";
            populateSubstrate(individual);
            lastIndividualSeen = individual;
        }

        drawContext.Clear();

        //THIS_SECTION_DRAWS_THE_GUI_FOR_A_PARTICULAR_EXPERIMENT
    }

    bool AIDSBalancedExperiment::handleMousePress(wxMouseEvent& event, wxSize& bitmapSize)
    {
        //THIS_SECTION_HANDLES_A_GUI_MOUSE_PRESS

        return true;
    }
#endif

    Experiment* AIDSBalancedExperiment::clone()
    {
        //THIS_SECTION_CLONES_THE_EXPERIMENT_SO_IT_CAN_BE_RUN_BY_MULTIPLE_THREADS_IN_PARALLEL

        AIDSBalancedExperiment* experiment = new AIDSBalancedExperiment(*this);

        return experiment;
    }

    void AIDSBalancedExperiment::resetGenerationData(boost::shared_ptr<NEAT::GeneticGeneration> generation)
    {
        //THIS_SECTION_CLEARS_GENERATION_STATISTICS
    }

    void AIDSBalancedExperiment::addGenerationData(boost::shared_ptr<NEAT::GeneticGeneration> generation, boost::shared_ptr<NEAT::GeneticIndividual> individual)
    {
        //THIS_SECTION_UPDATES_GENERATION_STATISTICS_AFTER_EACH_INDIVIDUAL_EVALUATION
    }
}
