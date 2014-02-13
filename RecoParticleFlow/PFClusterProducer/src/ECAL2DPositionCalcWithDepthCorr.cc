#include "ECAL2DPositionCalcWithDepthCorr.h"

#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include <cmath>
#include <unordered_map>

#include "DataFormats/EcalDetId/interface/EcalSubdetector.h"
#include "Geometry/CaloGeometry/interface/CaloCellGeometry.h"
#include "Geometry/CaloGeometry/interface/TruncatedPyramid.h"

// faithful reimplementation of RecoEcal/EgammaCoreTools PositionCalc
// sorry Stefano

void ECAL2DPositionCalcWithDepthCorr::
update(const edm::EventSetup& es) {
  const CaloGeometryRecord& temp = es.get<CaloGeometryRecord>();
  if( caloGeom == NULL || 
      ( caloGeom->cacheIdentifier() != temp.cacheIdentifier() ) ) {
    caloGeom = &temp;
    edm::ESHandle<CaloGeometry> geohandle;
    caloGeom->get(geohandle);
    _ebGeom = geohandle->getSubdetectorGeometry(DetId::Ecal,EcalBarrel);
    _eeGeom = geohandle->getSubdetectorGeometry(DetId::Ecal,EcalEndcap);
    _esGeom = geohandle->getSubdetectorGeometry(DetId::Ecal,EcalPreshower);
    // ripped from RecoEcal/EgammaCoreTools 
    for( uint32_t ic = 0; 
	 ic != _esGeom->getValidDetIds().size() && 
	   ( !_esPlus || !_esMinus ); ++ic ) {
	const double z = _esGeom->getGeometry( _esGeom->getValidDetIds()[ic] )->getPosition().z();
	_esPlus = _esPlus || ( 0 < z ) ;
	_esMinus = _esMinus || ( 0 > z ) ;
    }  
  }
}

void ECAL2DPositionCalcWithDepthCorr::
calculateAndSetPosition(reco::PFCluster& cluster) {
  calculateAndSetPositionActual(cluster);
}

void ECAL2DPositionCalcWithDepthCorr::
calculateAndSetPositions(reco::PFClusterCollection& clusters) {
  for( reco::PFCluster& cluster : clusters ) {
    calculateAndSetPositionActual(cluster);
  }
}

void ECAL2DPositionCalcWithDepthCorr::
calculateAndSetPositionActual(reco::PFCluster& cluster) const {  
  constexpr double preshowerStartEta =  1.653;
  if( !cluster.seed() ) {
    throw cms::Exception("ClusterWithNoSeed")
      << " Found a cluster with no seed: " << cluster;
  }  				
  double cl_energy = 0;  
  double max_e = 0.0;  
  double clusterT0 = 0.0;
  PFLayer::Layer max_e_layer = PFLayer::NONE;
  reco::PFRecHitRef refseed;  
  // find the seed and max layer
  for( const reco::PFRecHitFraction& rhf : cluster.recHitFractions() ) {
    const reco::PFRecHitRef& refhit = rhf.recHitRef();
    if( refhit->detId() == cluster.seed() ) refseed = refhit;
    const double rh_energy = refhit->energy() * rhf.fraction();    
    if( std::isnan(rh_energy) ) {
      throw cms::Exception("PFClusterAlgo")
	<<"rechit " << refhit->detId() << " has a NaN energy... " 
	<< "The input of the particle flow clustering seems to be corrupted.";
    }
    cl_energy += rh_energy;    
    if( rh_energy > max_e ) {
      max_e = rh_energy;
      max_e_layer = rhf.recHitRef()->layer();
    }    
  }
  cluster.setEnergy(cl_energy);
  cluster.setLayer(max_e_layer);
  const CaloSubdetectorGeometry* ecal_geom = NULL;
  // get seed geometry information
  const CaloCellGeometry* center_cell = 
    ecal_geom->getGeometry(refseed->detId());
  const double ctreta = center_cell->getPosition().eta();
  switch(max_e_layer){
  case PFLayer::ECAL_BARREL:
    ecal_geom = _ebGeom;
    clusterT0 = _param_T0_EB;
    break;
  case PFLayer::ECAL_ENDCAP:
    ecal_geom = _eeGeom;
    clusterT0 = _param_T0_EE;
    if( std::abs(ctreta) > preshowerStartEta ) {
      if(ctreta > 0 && _esPlus ) clusterT0 = _param_T0_ES;
      if(ctreta < 0 && _esMinus) clusterT0 = _param_T0_ES;
    }      
    break;
  default:
    throw cms::Exception("InvalidLayer")
      << "ECAL Position Calc only accepts ECAL_BARREL or ECAL_ENDCAP";
  }
  const double maxDepth = _param_X0*(clusterT0 + std::log(cluster.energy()));
  const double maxToFront = center_cell->getPosition().mag();
  const double logETot_inv = -std::log(cluster.energy());
  // calculate the position
  double position_norm = 0.0;
  double x(0.0),y(0.0),z(0.0);
  
  for( const reco::PFRecHitFraction& rhf : cluster.recHitFractions() ) {
    double weight = 0.0;
    const reco::PFRecHitRef& refhit = rhf.recHitRef();
    const double rh_energy = refhit->energy() * rhf.fraction();
    if( rh_energy > 0.0 ) weight = std::max(0.0,( _param_W0 + 
						  std::log(rh_energy) + 
						  logETot_inv ));
    const CaloCellGeometry* cell = ecal_geom->getGeometry(refhit->detId());
    const float depth = maxDepth + maxToFront - cell->getPosition().mag();    
    const GlobalPoint pos =
      static_cast<const TruncatedPyramid*>(cell)->getPosition(depth);
    
    x += weight*pos.x() ;
    y += weight*pos.y() ;
    z += weight*pos.z() ;
    
    position_norm += weight ;
  }
  if( position_norm < _minAllowedNorm ) {
    edm::LogError("WeirdClusterNormalization") 
      << "PFCluster too far from seeding cell: set position to (0,0,0).";
    cluster.setPosition(math::XYZPoint(x,y,z));
  } else {
    const double norm_inverse = 1.0/position_norm;
    x *= norm_inverse;
    y *= norm_inverse;
    z *= norm_inverse;
    cluster.setPosition(math::XYZPoint(x,y,z));
    cluster.calculatePositionREP();
  }
}
