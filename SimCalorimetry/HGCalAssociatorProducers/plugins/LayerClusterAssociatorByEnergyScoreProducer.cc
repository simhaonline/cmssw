// Original author: Marco Rovere

// user include files
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/global/EDProducer.h"

#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"

#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "FWCore/Utilities/interface/EDGetToken.h"

#include "SimDataFormats/Associations/interface/LayerClusterToCaloParticleAssociator.h"
#include "LayerClusterAssociatorByEnergyScoreImpl.h"

class LayerClusterAssociatorByEnergyScoreProducer : public edm::global::EDProducer<> {
public:
  explicit LayerClusterAssociatorByEnergyScoreProducer(const edm::ParameterSet &);
  ~LayerClusterAssociatorByEnergyScoreProducer() override;

  static void fillDescriptions(edm::ConfigurationDescriptions &descriptions);

private:
  void produce(edm::StreamID, edm::Event &, const edm::EventSetup &) const override;
  edm::EDGetTokenT<std::map<DetId, const HGCRecHit*>> hitMapToken_;
  const bool hardScatterOnly_;
  const int layers_;
  std::shared_ptr<hgcal::RecHitTools> rhtools_;
};

LayerClusterAssociatorByEnergyScoreProducer::LayerClusterAssociatorByEnergyScoreProducer(const edm::ParameterSet &ps)
  : hitMapToken_(consumes<std::map<DetId, const HGCRecHit*>>(ps.getParameter<edm::InputTag>("hitMapTag"))),
    hardScatterOnly_(ps.getParameter<bool>("hardScatterOnly")),
    layers_(ps.getParameter<int>("layers")) {
  rhtools_.reset(new hgcal::RecHitTools());

  // Register the product
  produces<hgcal::LayerClusterToCaloParticleAssociator>();
}

LayerClusterAssociatorByEnergyScoreProducer::~LayerClusterAssociatorByEnergyScoreProducer() {}

void LayerClusterAssociatorByEnergyScoreProducer::produce(edm::StreamID, edm::Event &iEvent, const edm::EventSetup &es) const {
  rhtools_->getEventSetup(es);
  edm::Handle<std::map<DetId, const HGCRecHit*>> hitMap;
  iEvent.getByToken(hitMapToken_, hitMap);
  const auto hitMapPointer = *hitMap;

  //std::unique_ptr<hgcal::LayerClusterToCaloParticleAssociatorBaseImpl> impl(new hgcal::LayerClusterToCaloParticleAssociatorBaseImpl());
  std::unique_ptr<hgcal::LayerClusterToCaloParticleAssociatorBaseImpl> impl(
    new LayerClusterAssociatorByEnergyScoreImpl(iEvent.productGetter(),
						hardScatterOnly_,
						rhtools_,
						&hitMapPointer,
						layers_));
  std::unique_ptr<hgcal::LayerClusterToCaloParticleAssociator> toPut(
      new hgcal::LayerClusterToCaloParticleAssociator(std::move(impl)));
  iEvent.put(std::move(toPut));
}

void LayerClusterAssociatorByEnergyScoreProducer::fillDescriptions(edm::ConfigurationDescriptions &cfg) {
  edm::ParameterSetDescription desc;
  desc.add<edm::InputTag>("hitMapTag", edm::InputTag("HGCRecHitMapProducer"));
  desc.add<bool>("hardScatterOnly", true);
  desc.add<int>("layers", 50);

  cfg.add("layerClusterAssociatorByEnergyScore", desc);
}

//define this as a plug-in
DEFINE_FWK_MODULE(LayerClusterAssociatorByEnergyScoreProducer);
