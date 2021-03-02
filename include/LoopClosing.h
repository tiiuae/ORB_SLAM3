/**
 * This file is part of ORB-SLAM3
 *
 * Copyright (C) 2017-2020 Carlos Campos, Richard Elvira, Juan J. Gómez Rodríguez, José M.M. Montiel and Juan D. Tardós,
 * University of Zaragoza. Copyright (C) 2014-2016 Raúl Mur-Artal, José M.M. Montiel and Juan D. Tardós, University of
 * Zaragoza.
 *
 * ORB-SLAM3 is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ORB-SLAM3 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with ORB-SLAM3.
 * If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LOOPCLOSING_H
#define LOOPCLOSING_H

#include "Atlas.h"
#include "KeyFrame.h"
#include "KeyFrameDatabase.h"
#include "LocalMapping.h"
#include "ORBVocabulary.h"
#include "Thirdparty/g2o/g2o/types/types_seven_dof_expmap.h"
#include "Tracking.h"
#include <boost/algorithm/string.hpp>
#include <mutex>
#include <thread>

namespace ORB_SLAM3
{

class Tracking;
class LocalMapping;
class KeyFrameDatabase;
class Map;

class LoopClosing
{
  public:
    typedef pair<set<KeyFrame*>, int> ConsistentGroup;
    typedef map<KeyFrame*,
                g2o::Sim3,
                std::less<KeyFrame*>,
                Eigen::aligned_allocator<std::pair<KeyFrame* const, g2o::Sim3> > >
        KeyFrameAndPose;

  public:
    LoopClosing(std::shared_ptr<Atlas> pAtlas,
                KeyFrameDatabase* pDB,
                std::shared_ptr<ORBVocabulary> pVoc,
                const bool bFixScale);

    void SetTracker(std::shared_ptr<Tracking>& pTracker);
    void SetLocalMapper(std::shared_ptr<LocalMapping>& pLocalMapper);

    // Main function
    void Run();

    void InsertKeyFrame(KeyFrame* pKF);

    void RequestReset();
    void RequestResetActiveMap(Map* pMap);

    // This function will run in a separate thread
    void RunGlobalBundleAdjustment(Map* pActiveMap, unsigned long nLoopKF);

    bool isRunningGBA()
    {
        unique_lock<std::mutex> lock(mMutexGBA);
        return mbRunningGBA;
    }
    bool isFinishedGBA()
    {
        unique_lock<std::mutex> lock(mMutexGBA);
        return mbFinishedGBA;
    }

    void RequestFinish();

    bool isFinished();

    Viewer* mpViewer;

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  protected:
    bool CheckNewKeyFrames();

    // Methods to implement the new place recognition algorithm
    bool NewDetectCommonRegions();
    bool DetectAndReffineSim3FromLastKF(KeyFrame* pCurrentKF,
                                        KeyFrame* pMatchedKF,
                                        g2o::Sim3& gScw,
                                        int& nNumProjMatches,
                                        std::vector<MapPoint*>& vpMPs,
                                        std::vector<MapPoint*>& vpMatchedMPs);
    bool DetectCommonRegionsFromBoW(std::vector<KeyFrame*>& vpBowCand,
                                    KeyFrame*& pMatchedKF,
                                    KeyFrame*& pLastCurrentKF,
                                    g2o::Sim3& g2oScw,
                                    int& nNumCoincidences,
                                    std::vector<MapPoint*>& vpMPs,
                                    std::vector<MapPoint*>& vpMatchedMPs);
    bool DetectCommonRegionsFromLastKF(KeyFrame* pCurrentKF,
                                       KeyFrame* pMatchedKF,
                                       g2o::Sim3& gScw,
                                       int& nNumProjMatches,
                                       std::vector<MapPoint*>& vpMPs,
                                       std::vector<MapPoint*>& vpMatchedMPs);
    int FindMatchesByProjection(KeyFrame* pCurrentKF,
                                KeyFrame* pMatchedKFw,
                                g2o::Sim3& g2oScw,
                                set<MapPoint*>& spMatchedMPinOrigin,
                                vector<MapPoint*>& vpMapPoints,
                                vector<MapPoint*>& vpMatchedMapPoints);

    void SearchAndFuse(const KeyFrameAndPose& CorrectedPosesMap, vector<MapPoint*>& vpMapPoints);
    void SearchAndFuse(const vector<KeyFrame*>& vConectedKFs, vector<MapPoint*>& vpMapPoints);

    void CorrectLoop();

    void MergeLocal();
    void MergeLocal2();

    void CheckObservations(set<KeyFrame*>& spKFsMap1, set<KeyFrame*>& spKFsMap2);
    void printReprojectionError(set<KeyFrame*>& spLocalWindowKFs, KeyFrame* mpCurrentKF, string& name);

    void ResetIfRequested();
    bool mbResetRequested;
    bool mbResetActiveMapRequested;
    Map* mpMapToReset;
    std::mutex mMutexReset;

    bool CheckFinish();
    void SetFinish();
    bool mbFinishRequested;
    bool mbFinished;
    std::mutex mMutexFinish;

    std::shared_ptr<Atlas> mpAtlas;
    std::shared_ptr<Tracking> mpTracker;

    KeyFrameDatabase* mpKeyFrameDB;
    std::shared_ptr<ORBVocabulary> mpORBVocabulary;

    std::shared_ptr<LocalMapping> mpLocalMapper;

    std::list<KeyFrame*> mlpLoopKeyFrameQueue;

    std::mutex mMutexLoopQueue;

    // Loop detector parameters
    float mnCovisibilityConsistencyTh;

    // Loop detector variables
    KeyFrame* mpCurrentKF;
    KeyFrame* mpLastCurrentKF;
    KeyFrame* mpMatchedKF;
    std::vector<ConsistentGroup> mvConsistentGroups;
    std::vector<KeyFrame*> mvpEnoughConsistentCandidates;
    std::vector<KeyFrame*> mvpCurrentConnectedKFs;
    std::vector<MapPoint*> mvpCurrentMatchedPoints;
    std::vector<MapPoint*> mvpLoopMapPoints;
    cv::Mat mScw;
    g2o::Sim3 mg2oScw;

    //-------
    Map* mpLastMap;

    bool mbLoopDetected;
    int mnLoopNumCoincidences;
    int mnLoopNumNotFound;
    KeyFrame* mpLoopLastCurrentKF;
    g2o::Sim3 mg2oLoopSlw;
    g2o::Sim3 mg2oLoopScw;
    KeyFrame* mpLoopMatchedKF;
    std::vector<MapPoint*> mvpLoopMPs;
    std::vector<MapPoint*> mvpLoopMatchedMPs;
    bool mbMergeDetected;
    int mnMergeNumCoincidences;
    int mnMergeNumNotFound;
    KeyFrame* mpMergeLastCurrentKF;
    g2o::Sim3 mg2oMergeSlw;
    g2o::Sim3 mg2oMergeSmw;
    g2o::Sim3 mg2oMergeScw;
    KeyFrame* mpMergeMatchedKF;
    std::vector<MapPoint*> mvpMergeMPs;
    std::vector<MapPoint*> mvpMergeMatchedMPs;
    std::vector<KeyFrame*> mvpMergeConnectedKFs;

    g2o::Sim3 mSold_new;
    //-------

    long unsigned int mLastLoopKFid;

    // Variables related to Global Bundle Adjustment
    bool mbRunningGBA;
    bool mbFinishedGBA;
    bool mbStopGBA;
    std::mutex mMutexGBA;
    std::thread* mpThreadGBA;

    // Fix scale in the stereo/RGB-D case
    bool mbFixScale;

    bool mnFullBAIdx;

    vector<double> vdPR_CurrentTime;
    vector<double> vdPR_MatchedTime;
    vector<int> vnPR_TypeRecogn;
};

}  // namespace ORB_SLAM3

#endif  // LOOPCLOSING_H
