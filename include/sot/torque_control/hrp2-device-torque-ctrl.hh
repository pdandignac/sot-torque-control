/*
 * Copyright 2014, Andrea Del Prete, LAAS-CNRS
 *
 * This file is part of sot-torque-control.
 * sot-torque-control is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 * sot-torque-control is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.  You should
 * have received a copy of the GNU Lesser General Public License along
 * with sot-torque-control.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _HRP2DeviceTorqueCtrl_H_
#define _HRP2DeviceTorqueCtrl_H_

#include <dynamic-graph/entity.h>
#include <dynamic-graph/signal.h>
#include <dynamic-graph/signal-ptr.h>
#include <dynamic-graph/linear-algebra.h>
#include <sot/core/device.hh>
#include <sot/core/abstract-sot-external-interface.hh>
#include <sot/core/matrix-rotation.hh>

#include <sot/torque_control/signal-helper.hh>
#include <sot/torque_control/hrp2-common.hh>
#include <sot/torque_control/utils/logger.hh>

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/variate_generator.hpp>

#include <pininvdyn/robot-wrapper.hpp>
#include <pininvdyn/tasks/task-se3-equality.hpp>

#include <Eigen/Cholesky>

namespace dgsot=dynamicgraph::sot;

namespace dynamicgraph
{
  namespace sot
  {
    namespace torque_control
    {

      /** Version of HRP2 device for testing the code without the
  * real robot. This version of the device assumes that the robot
  * is torque controlled (i.e. the control input are the desired
  * joint torques). These desired joint torques are used to compute
  * the joint accelerations and the contact forces.
  * The joint accelerations are then integrated to get the
  * measured joint angles.
  *
  * The feet are supposed to be fixed to the ground. The accelerometer
  * and the gyrometer output a constant value.
  *
  * A white Gaussian noise is added to the force/torque sensor
  * measurements.
  *
  * TODO: The original Device class should be a clean abstraction for
  * the concept of device, but currently it is not. It defines a lot
  * of input/output signals that are specific of HRP-2 (e.g. zmpSIN,
  * attitudeSIN, forcesSOUT) and some design choices (e.g. secondOrderIntegration).
  * It would be nice to clean the original Device from all this stuff
  * and move it in the specific subclasses.
  */
      class HRP2DeviceTorqueCtrl: public
          dgsot::Device
      {
      public:

        static const std::string CLASS_NAME;
        static const double TIMESTEP_DEFAULT;
        static const double FORCE_SENSOR_NOISE_STD_DEV;

        virtual const std::string& getClassName () const
        {
          return CLASS_NAME;
        }

        HRP2DeviceTorqueCtrl(std::string RobotName);
        virtual ~HRP2DeviceTorqueCtrl();

        virtual void setStateSize(const unsigned int& size);
        virtual void setState(const ml::Vector& st);
        virtual void setVelocity(const ml::Vector & vel);

        virtual void init(const double& dt, const std::string& urdfFile);

      protected:
        virtual void integrate(const double & dt);

        void sendMsg(const std::string& msg, MsgType t=MSG_TYPE_INFO, const char* file="", int line=0)
        {
          getLogger().sendMsg("[HRP2DeviceTorqueCtrl] "+msg, t, file, line);
        }

        /// \brief Current integration step.
        double timestep_;
        bool m_initSucceeded;

        /// input force sensor values
        dynamicgraph::SignalPtr <ml::Vector, int>* forcesSIN_[4];

        /// Accelerations measured by accelerometers
        dynamicgraph::Signal <ml::Vector, int> accelerometerSOUT_;
        /// Rotation velocity measured by gyrometers
        dynamicgraph::Signal <ml::Vector, int> gyrometerSOUT_;
        /// base 6d pose + joints' angles measured by encoders
        dynamicgraph::Signal <ml::Vector, int> robotStateSOUT_;
        /// joints' velocities computed by the integrator
        dynamicgraph::Signal <ml::Vector, int> jointsVelocitiesSOUT_;
        /// joints' accelerations computed by the integrator
        dynamicgraph::Signal <ml::Vector, int> jointsAccelerationsSOUT_;
        /// motor currents
        dynamicgraph::Signal <ml::Vector, int> currentSOUT_;
        /// proportional and derivative position-control gains
        dynamicgraph::Signal <ml::Vector, int> p_gainsSOUT_;
        dynamicgraph::Signal <ml::Vector, int> d_gainsSOUT_;

        DECLARE_SIGNAL_IN(kp_constraints,             ml::Vector);
        DECLARE_SIGNAL_IN(kd_constraints,             ml::Vector);
        DECLARE_SIGNAL_IN(rotor_inertias,             ml::Vector);
        DECLARE_SIGNAL_IN(gear_ratios,                ml::Vector);

        /// Intermediate variables to avoid allocation during control
        ml::Vector accelerometer_;
        ml::Vector gyrometer_;

        ml::Vector base6d_encoders_;      /// base 6d pose + joints' angles
        ml::Vector jointsVelocities_;     /// joints' velocities
        ml::Vector jointsAccelerations_;  /// joints' accelerations

        ml::Vector wrenches_[4];
        ml::Vector temp6_;

        /// robot geometric/inertial data
        pininvdyn::RobotWrapper *                       m_robot;
        se3::Data *                                     m_data;
        pininvdyn::tasks::TaskSE3Equality *             m_contactRF;
        pininvdyn::tasks::TaskSE3Equality *             m_contactLF;
        unsigned int                                    m_nk; // number of contact forces

        pininvdyn::math::Vector m_q, m_v, m_dv, m_f;
        pininvdyn::math::Vector m_q_sot, m_v_sot, m_dv_sot;

        typedef Eigen::LDLT<Eigen::MatrixXd> Cholesky;
        Cholesky        m_K_chol; /// cholesky decomposition of the K matrix
        Eigen::MatrixXd m_K;
        Eigen::VectorXd m_k;
        Eigen::MatrixXd m_Jc; /// constraint Jacobian

        typedef boost::mt11213b                    ENG;    // uniform random number generator
        typedef boost::normal_distribution<double> DIST;   // Normal Distribution
        typedef boost::variate_generator<ENG,DIST> GEN;    // Variate generator
        ENG  randomNumberGenerator_;
        DIST normalDistribution_;
        GEN  normalRandomNumberGenerator_;
      };

    }   // end namespace torque_control
  }   // end namespace sot
}   // end namespace dynamicgraph
#endif /* HRP2DevicePosCtrl*/