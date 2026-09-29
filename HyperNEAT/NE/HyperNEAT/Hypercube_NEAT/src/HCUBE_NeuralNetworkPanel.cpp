#include "HCUBE_Defines.h"

#ifndef HCUBE_NOGUI

#include "HCUBE_NeuralNetworkPanel.h"

#include "HCUBE_NetworkPanel.h"
#include "HCUBE_NeuralNetworkWeightGrid.h"

namespace HCUBE
{

    using namespace NEAT;

    NeuralNetworkPanel::NeuralNetworkPanel( wxWindow* parent, int id, wxPoint pos, wxSize size, int style )
            :
            NeuralNetworkPanelTemplate( parent, id, pos, size, style )//,
            //outputBitmap(size.x,size.y)
    {}

    void NeuralNetworkPanel::setTarget(boost::shared_ptr<NEAT::GeneticIndividual> individual)
    {
        networkVisualizationPanel->createNetworkImage(individual);
        networkAdjacency->setTarget(individual);
    }

    void NeuralNetworkPanel::setReadOnly(bool value)
    {
        networkAdjacency->setReadOnly(value);
    }

    bool NeuralNetworkPanel::isReadOnly()
    {
        return networkAdjacency->isReadOnly();
    }

}

#endif

