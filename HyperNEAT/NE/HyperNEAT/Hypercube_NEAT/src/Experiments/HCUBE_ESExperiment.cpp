#include "HCUBE_Defines.h"

#include "Experiments/HCUBE_ESExperiment.h"
#include "cppn.h"
#include "esNodeExplorer.h"
#include "esLinkExplorer.h"

#include <sstream>

#define DO_REGULAR_RUN_FOR_POSTHOC (1)

// Inputs are constant nodes, so after N steps every node within N links of the
// inputs holds its feed-forward value. ES can chain hidden nodes, so use more
// than the 2 steps a single fixed hidden layer needs.
#define ES_ACTIVATION_STEPS (4)

// Constant node (value 1) that feeds each hidden node its ES bias.
#define ES_BIAS_NODE "ESBias"

//DATASET PROCESSING FUNCTION DECLARATIONS
// Kept file-local (anonymous namespace) so they don't clash with the identical
// helpers in HCUBE_AIDSExploitExperiment.cpp at link time.
namespace {
int getCSVRowCount(fstream&);
int getCSVColumnCount(string&);
void getCSVRowTokens(string&, vector<string>&);
void printStringVector(vector<string>&);
void printStringVectorVector(vector<vector<string>>&);
bool isString(string&);
bool isIndexInVector(int, vector<int>&);
int getIndexFromVector(int, vector<int>&);
bool isStringInVector(string&, vector<string>&);
}


namespace HCUBE
{
    using namespace NEAT;

    ESExperiment::ESExperiment(string _experimentName, int _threadID)
        :
        Experiment(_experimentName, _threadID)
    {
        //process dataset here

        //dataset processing code
        fstream infile;
        //fstream outfile;

        cout << "Creating FINDME.txt" << endl;
        ofstream{ "FINDME.txt" };

        infile.open("subset.csv");
        if (!infile.is_open())
        {
            cout << "Could not open file\n";
        }

        const int row_count = getCSVRowCount(infile);
        cout << "File's line count: " << getCSVRowCount(infile) << endl;
        if (row_count > 1000000)
        {
            cout << "Input dataset exceeds maximum number of rows. Aborting.\n";
        }

        string buffer_string;
        vector<string> header_tokens;
        vector<vector<float>> csv_data;

        getline(infile, buffer_string);
        getCSVRowTokens(buffer_string, header_tokens);

        const int column_count = header_tokens.size();
        cout << "Number of columns is: " << column_count << endl << endl;

        int iterator = 0;
        while (!infile.eof())
        {
            //get string
            getline(infile, buffer_string);

            vector<string> tokens;
            vector<float> token_values;
            getCSVRowTokens(buffer_string, tokens); //get values from string

            //check for mismatched column count in row
            if (tokens.size() == column_count)
            {
                bool input_error = false;

                //check for non-string values
                for (int i = 0; i < column_count; i++)
                {
                    if (!isString(tokens.at(i)))
                    {
                        token_values.push_back(stof(tokens.at(i)));
                    }
                    else
                    {
                        input_error = true;
                        i = column_count;
                    }
                }
                //if no errors, add data
                if (!input_error)
                {
                    csv_data.push_back(token_values);
                }
                else
                {
                    cout << "Non-string value found in line " << iterator << ", skipping line." << endl;
                }
            }

            iterator++;
        }
        infile.close();

        cout << "Data loaded" << endl << "Forming data subsets" << endl;

        vector<string> input_headers;
        vector<string> output_headers;
        vector<vector<float>> input_data;
        vector<vector<float>> output_data;

        //this makes massive assumptions about the structure of the dataset
        int io_column_pivot;

        for (int i = 0; i < header_tokens.size(); i++)
        {
            if (header_tokens.at(i).find("labels") != std::string::npos)
            {
                io_column_pivot = i;
                i = header_tokens.size();
            }
        }

        for (int i = 0; i < io_column_pivot; i++)
        {
            input_headers.push_back(header_tokens.at(i));
        }
        for (int i = io_column_pivot; i < header_tokens.size(); i++)
        {
            output_headers.push_back(header_tokens.at(i));
        }

        for (int i = 0; i < csv_data.size(); i++)
        {
            vector<float> float_buffer;

            for (int j = 0; j < io_column_pivot; j++)
            {
                float_buffer.push_back(csv_data.at(i).at(j));
            }
            data.push_back(float_buffer);

            float_buffer.clear();

            for (int j = io_column_pivot; j < header_tokens.size(); j++)
            {
                float_buffer.push_back(csv_data.at(i).at(j));
            }
            answers.push_back(float_buffer);
        }

        dataNames = header_tokens;

        numNodesIn = io_column_pivot;
        numNodesOut = header_tokens.size() - io_column_pivot;
        numScenarios = row_count - 1;

        generateSubstrate();
    }

    ESExperiment::~ESExperiment()
    {
    }

    GeneticPopulation* ESExperiment::createInitialPopulation(int populationSize)
    {
        GeneticPopulation* population = new GeneticPopulation();
        vector<GeneticNodeGene> genes;

        // CPPN inputs/outputs; names must match ES::CPPN (src/ES/cppn.cpp).
        genes.push_back(GeneticNodeGene("Bias", "NetworkSensor", 0, false));
        genes.push_back(GeneticNodeGene("X1", "NetworkSensor", 0, false));
        genes.push_back(GeneticNodeGene("Y1", "NetworkSensor", 0, false));
        genes.push_back(GeneticNodeGene("X2", "NetworkSensor", 0, false));
        genes.push_back(GeneticNodeGene("Y2", "NetworkSensor", 0, false));
        genes.push_back(GeneticNodeGene("D", "NetworkSensor", 0, false));
        genes.push_back(GeneticNodeGene("Angle", "NetworkSensor", 0, false));
        genes.push_back(GeneticNodeGene("LinkGate", "NetworkOutputNode", 1, false, ACTIVATION_FUNCTION_SIGMOID));
        genes.push_back(GeneticNodeGene("LinkWeight", "NetworkOutputNode", 1, false, ACTIVATION_FUNCTION_SIGMOID));
        genes.push_back(GeneticNodeGene("NodeGate", "NetworkOutputNode", 1, false, ACTIVATION_FUNCTION_SIGMOID));
        genes.push_back(GeneticNodeGene("NodeBias", "NetworkOutputNode", 1, false, ACTIVATION_FUNCTION_SIGMOID));


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

    void ESExperiment::generateSubstrate()
    {
        // Only the inputs and outputs are fixed: inputs spread along the bottom edge
        // of [-1,1]^2 and outputs along the top. The hidden nodes and every link are
        // chosen per individual by the ES explorers in populateSubstrate().
        cout << "Generating ES substrate layout...";

        layout = ES::Substrate();

        for (int i = 0; i < numNodesIn; i++)
        {
            ES::Node n{};
            n.ID = i;
            n.isInput = true;
            n.pos = { numNodesIn > 1 ? -1.f + 2.f * i / (numNodesIn - 1) : 0.f, -1.f };
            layout.inputs.push_back(n);
        }

        for (int i = 0; i < numNodesOut; i++)
        {
            ES::Node n{};
            n.ID = numNodesIn + i;
            n.isOutput = true;
            n.pos = { numNodesOut > 1 ? -1.f + 2.f * i / (numNodesOut - 1) : 0.f, 1.f };
            layout.outputs.push_back(n);
        }

        cout << "done!\n";
    }

    void ESExperiment::populateSubstrate(boost::shared_ptr<NEAT::GeneticIndividual> individual)
    {
        NEAT::FastNetwork<double> network = individual->spawnFastPhenotypeStack<double>();
        ES::CPPN cppn(&network);

        // Explorer settings are the defaults, which match Slager's runESFull.cpp.
        ES::Substrate sub = layout;
        ES::ESNodeConfig nodeConfig;
        ES::ESNodeExplorer(nodeConfig).discoverHiddenNodes(cppn, sub);

        vector<ES::Link> esLinks;
        ES::ESLinkConfig linkConfig;
        ES::ESLinkExplorer(linkConfig).removeLinks(cppn, sub, esLinks);

        // Build the substrate network. Inputs and the bias source are constant
        // (update=false) so their values hold across the activation steps.
        vector<NetworkNode> nodes;
        nodes.reserve(sub.inputs.size() + 1 + sub.outputs.size() + sub.hiddens.size());
        map<int, int> idToIndex;

        for (int i = 0; i < (int)sub.inputs.size(); i++)
        {
            idToIndex[sub.inputs[i].ID] = nodes.size();
            nodes.push_back(NetworkNode(dataNames[i], false));
        }

        const int biasIndex = nodes.size();
        nodes.push_back(NetworkNode(ES_BIAS_NODE, false));

        for (int i = 0; i < (int)sub.outputs.size(); i++)
        {
            idToIndex[sub.outputs[i].ID] = nodes.size();
            nodes.push_back(NetworkNode(dataNames[numNodesIn + i], true));
        }

        for (const ES::Node& h : sub.hiddens)
        {
            idToIndex[h.ID] = nodes.size();
            nodes.push_back(NetworkNode(string("h") + toString(h.ID), true));
        }

        map<int, int> fanIn;
        for (const ES::Link& l : esLinks)
        {
            fanIn[l.postID]++;
        }

        vector<NetworkLink> links;
        links.reserve(esLinks.size() + sub.hiddens.size());

        for (const ES::Link& l : esLinks)
        {
            // Same fan-in scaling as AIDSExploit, so summed inputs don't saturate.
            double weight = l.w * 3.0 / sqrt(double(fanIn[l.postID]));
            links.push_back(NetworkLink(&nodes[idToIndex[l.preID]], &nodes[idToIndex[l.postID]], weight));
        }

        for (const ES::Node& h : sub.hiddens)
        {
            if (h.bias != 0.f)
            {
                links.push_back(NetworkLink(&nodes[biasIndex], &nodes[idToIndex[h.ID]], double(h.bias)));
            }
        }

        substrate = NEAT::FastNetwork<double>(nodes.data(), nodes.size(), links.data(), links.size());

        cout << "ES hidden=" << sub.hiddens.size() << " links=" << esLinks.size() << "\n";
    }

    void ESExperiment::processGroup(boost::shared_ptr<NEAT::GeneticGeneration> generation)
    {
        boost::shared_ptr<NEAT::GeneticIndividual> individual = group.front();
        individual->setFitness(0);

        populateSubstrate(individual);

        int scenario, i;
        vector<vector<float>> outputs;

        for (scenario = 0; scenario < numScenarios; scenario++) {
            // Reset per row so no state carries over from the previous row.
            substrate.reinitialize();
            substrate.dummyActivation();

            for (i = 0; i < numNodesIn; i++) {

                substrate.setValue(dataNames[i], data[scenario][i]);

            }
            substrate.setValue(ES_BIAS_NODE, 1.0);

            substrate.updateFixedIterations(ES_ACTIVATION_STEPS);

            vector<float> output_temp;
            for (i = 0; i < numNodesOut; i++) {

                output_temp.push_back(substrate.getValue(dataNames[numNodesIn + i]));
            }
            outputs.push_back(output_temp);
        }

        individual->reward(evaluateFitness(outputs));

    }

    double ESExperiment::evaluateFitness(vector<vector<float>> outputs) {

        int row, col; //rows are scenarios, cols are output node values
        //int numTypes = numNodesOut - 1; //the number of known intrusion types
        float output, answer;

        // The label is inverted for scoring (label 0 is treated as the positive
        // class). This used to flip answers[][] in place on every call, so
        // successive individuals were scored against alternating labels.
        // Invert a local copy instead so every evaluation sees the same data.

        float truePos = 0.0, trueNeg = 0.0, falsePos = 0.0, falseNeg = 0.0;
        float accuracy, truePosRate, falseAlarmRate, F1;
        float fitness;

        for (row = 0; row < numScenarios; row++) {
            output = outputs[row][0];
            answer = (answers[row][0] == 0) ? 1.0f : 0.0f;

            if ((output >= 0.5) && (answer == 0.0)) {       //NOTE: >=0.5 equivalent to ~1.0, <0.5 equivalent to ~0.0
                falsePos++;
            }
            else if ((output < 0.5) && (answer == 1.0)) {
                falseNeg++;
            }
            else if ((output >= 0.5) && (answer == 1.0)) {
                truePos++;
            }
            else if ((output < 0.5) && (answer == 0.0)) {
                trueNeg++;
            }
        }

        accuracy = (truePos + trueNeg) / (truePos + trueNeg + falsePos + falseNeg);

        truePosRate = (truePos + falseNeg) > 0 ? truePos / (truePos + falseNeg) : 0.0f;

        falseAlarmRate = (falsePos + truePos) > 0 ? falsePos / (falsePos + truePos) : 0.0f;

        F1 = (2.0 * truePos) / ((2.0 * truePos) + falsePos + falseNeg);

        // Balanced accuracy: mean of detection rate and true-negative rate.
        // The previous (accuracy + truePosRate) / 2 gave a network that flags every row
        // as an attack 75/100 on balanced data, so evolution got stuck there. Under
        // balanced accuracy any constant output scores exactly 50.
        float trueNegRate = (trueNeg + falsePos) > 0 ? trueNeg / (trueNeg + falsePos) : 0.0f;
        fitness = (truePosRate + trueNegRate) / 2.0;

        // One machine-readable line per evaluation, parsed by the web UI (ui/server.py).
        {
            std::ostringstream evalLine;
            evalLine << "EVAL tp=" << truePos << " tn=" << trueNeg << " fp=" << falsePos
                     << " fn=" << falseNeg << " fit=" << (fitness * 100.0) << "\n";
            cout << evalLine.str() << flush;
        }




        //if there are 10 known intrusion types, then the number of output nodes is 11
        //then if the last 10 cols match, the individual has correctly identified the kind of intrusion
        bool typeCorrect = true;
        float trueType = 0.0, falseType = 0.0;
        float typeAccuracy;

        for (row = 0; row < numScenarios; row++) {
            for (col = 1; col < numNodesOut; col++) {

                if (outputs[row][col] != answers[row][col]) {   //NOTE: THIS REQUIRES EXACTLY THE SAME ANSWER, PROBABLY WORTH CHANGING
                    typeCorrect = false;
                }
            }

            if (typeCorrect) {
                trueType++;
            }
            else {
                falseType++;
            }
        }

        typeAccuracy = trueType / (trueType + falseType);

        return (double)(fitness*100.0);
    }




    void ESExperiment::processIndividualPostHoc(boost::shared_ptr<NEAT::GeneticIndividual> individual)
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
    void ESExperiment::createIndividualImage(wxDC& drawContext, boost::shared_ptr<NEAT::GeneticIndividual> individual)
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

    bool ESExperiment::handleMousePress(wxMouseEvent& event, wxSize& bitmapSize)
    {
        //THIS_SECTION_HANDLES_A_GUI_MOUSE_PRESS

        return true;
    }
#endif

    Experiment* ESExperiment::clone()
    {
        //THIS_SECTION_CLONES_THE_EXPERIMENT_SO_IT_CAN_BE_RUN_BY_MULTIPLE_THREADS_IN_PARALLEL

        ESExperiment* experiment = new ESExperiment(*this);

        return experiment;
    }

    void ESExperiment::resetGenerationData(boost::shared_ptr<NEAT::GeneticGeneration> generation)
    {
        //THIS_SECTION_CLEARS_GENERATION_STATISTICS
    }

    void ESExperiment::addGenerationData(boost::shared_ptr<NEAT::GeneticGeneration> generation, boost::shared_ptr<NEAT::GeneticIndividual> individual)
    {
        //THIS_SECTION_UPDATES_GENERATION_STATISTICS_AFTER_EACH_INDIVIDUAL_EVALUATION
    }
}


// DATASET PROCESSING FUNCTIONS
namespace {

int getCSVRowCount(fstream& f)
{
    int count = 0;
    string buffer;

    // Reset stream to a good state at the beginning. Without this, calling
    // this function twice in a row (as the constructor does) spins forever:
    // the first call leaves failbit set, seekg's sentry then does nothing,
    // and the next `while(!f.eof())` never terminates (fail=1, eof=0).
    f.clear();
    f.seekg(0, f.beg);

    while (!f.eof())
    {
        count++;
        getline(f, buffer);
    }

    // Leave the stream usable (good state, positioned at start) for the caller.
    f.clear();
    f.seekg(0, f.beg);

    return count;
}

int getCSVColumnCount(string& s)
{
    int count = 0;
    int start = 0;
    int end = 0;

    while (end < s.size())
    {
        count++;

        end = s.find(',', start);

        start = end + 1;
    }

    return count;
}



void getCSVRowTokens(string& s, vector<string>& t)
{
    int start = 0;
    int end = 0;


    //debug
    //cout << "Running getCSVRowTokens on - " << s << endl;

    while (end < s.size())
    {
        end = s.find(',', start);
        t.push_back(s.substr(start, end - start));

        start = end + 1;
    }
}

void printStringVector(vector<string>& v)
{
    for (int i = 0; i < v.size(); i++)
    {
        cout << v.at(i) << ", ";
    }
    cout << endl;
}

void printStringVectorVector(vector<vector<string>>& vv)
{
    for (int i = 0; i < vv.size(); i++)
    {
        printStringVector(vv.at(i));
    }
}

bool isString(string& s)
{
    for (int i = 0; i < s.length(); i++)
    {
        // A leading minus sign is numeric, and a trailing '\r' comes from
        // Windows line endings; before, both made the whole row get skipped.
        if ((s[i] == '-' && i == 0) || (s[i] == '\r' && i == s.length() - 1))
        {
            continue;
        }
        //return false if a char in s is not between 46 and 57 and is not null-terminator
        if (!(s[i] <= 57 && s[i] >= 46) && s[i] != 0)
        {
            return true;
        }
    }

    return false;
}

bool isIndexInVector(int idx, vector<int>& indecies)
{
    for (int i = 0; i < indecies.size(); i++)
    {
        if (indecies.at(i) == idx) return true;
    }

    return false;
}

int getIndexFromVector(int idx, vector<int>& indecies)
{
    for (int i = 0; i < indecies.size(); i++)
    {
        if (indecies.at(i) == idx) return i;
    }

    return -1;
}

bool isStringInVector(string& s, vector<string>& sv)
{
    for (int i = 0; i < sv.size(); i++)
    {
        if (sv.at(i).compare(s) == 0) return true;
    }

    return false;
}

} // namespace