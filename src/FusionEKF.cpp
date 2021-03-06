#include "FusionEKF.h"
#include "tools.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/*
 * Constructor.
 */
FusionEKF::FusionEKF() {
  is_initialized_ = false;

  previous_timestamp_ = 0;

  // initializing matrices
  R_laser_ = MatrixXd(2, 2);
  R_radar_ = MatrixXd(3, 3);
  H_laser_ = MatrixXd(2, 4);
  Hj_ = MatrixXd(3, 4);

  //measurement covariance matrix - laser
  R_laser_ << 0.0225, 0,
        0, 0.0225;

  //measurement covariance matrix - radar
  R_radar_ << 0.09, 0, 0,
        0, 0.0009, 0,
        0, 0, 0.09;

  //state covariance matrix P
  MatrixXd P_init = MatrixXd(4, 4);
  P_init << 1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1000, 0,
            0, 0, 0, 1000;

  //the initial transition matrix F_
  MatrixXd F_init = MatrixXd(4, 4);
  F_init << 1, 0, 1, 0,
            0, 1, 0, 1,
            0, 0, 1, 0,
            0, 0, 0, 1;

  //Laser measures x and y directly, no velocity measured
  H_laser_<< 1, 0, 0, 0,
             0, 1, 0, 0;

  // Anything is OK on x_ and Q_ since those will be updated when we get the first measurements
  VectorXd x_init = VectorXd(4);
  x_init << 1, 1, 1, 1;
  MatrixXd Q_init = MatrixXd(4, 4);
  Q_init << 1, 0, 1, 0,
            0, 1, 0, 1,
            1, 0, 1, 0,
            0, 1, 0, 1;


  //Initialize the Kalman Filter
  ekf_.Init(x_init, P_init, F_init, H_laser_, R_laser_, Q_init);
  

  //set the acceleration noise components as instructed in the Prediction comments
  noise_ax = 9;
  noise_ay = 9;

}

/**
* Destructor.
*/
FusionEKF::~FusionEKF() {}

void FusionEKF::ProcessMeasurement(const MeasurementPackage &measurement_pack) {


  /*****************************************************************************
   *  Initialization
   ****************************************************************************/
  if (!is_initialized_) {
    /**
      * Initialize the state ekf_.x_ with the first measurement.
      * Create the covariance matrix.
      * Remember: you'll need to convert radar from polar to cartesian coordinates.
    */
    // first measurement
    cout << "EKF: " << endl;
    float px_init;
    float py_init;
    float vx_init = 0.0;
    float vy_init = 0.0;

    if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
      /**
      Convert radar from polar to cartesian coordinates and initialize state.
      */
      float r_init = measurement_pack.raw_measurements_[0];
      float theta_init = measurement_pack.raw_measurements_[1];
      px_init = r_init * cos(theta_init);
      py_init = r_init * sin(theta_init);
    }
    else if (measurement_pack.sensor_type_ == MeasurementPackage::LASER) {
      /**
      Initialize state.
      */
      px_init = measurement_pack.raw_measurements_[0];
      py_init = measurement_pack.raw_measurements_[1];
    }

    if (px_init < 0.0001 && py_init < 0.0001) return;

    ekf_.x_ << px_init, py_init, vx_init, vy_init;
    previous_timestamp_ = measurement_pack.timestamp_;
    // done initializing, no need to predict or update
    is_initialized_ = true;
    return;
  }

  /*****************************************************************************
   *  Prediction
   ****************************************************************************/

  /**
     * Update the state transition matrix F according to the new elapsed time.
      - Time is measured in seconds.
     * Update the process noise covariance matrix.
     * Use noise_ax = 9 and noise_ay = 9 for your Q matrix.
   */
    //compute the time elapsed between the current and previous measurements
    float dt = (measurement_pack.timestamp_ - previous_timestamp_) / 1000000.0; //dt - expressed in seconds
    previous_timestamp_ = measurement_pack.timestamp_;

    if(dt > 0.0001){
      float dt2 = dt * dt;
      float dt3 = dt2 * dt;
      float dt4 = dt3 * dt;

      //Modify the F matrix so that the time is integrated
      ekf_.F_(0, 2) = dt;
      ekf_.F_(1, 3) = dt;

      //set the process covariance matrix Q
      //kf_.Q_ = MatrixXd(4, 4);
      ekf_.Q_ <<  dt4/4*noise_ax, 0, dt3/2*noise_ax, 0,
                 0, dt4/4*noise_ay, 0, dt3/2*noise_ay,
                 dt3/2*noise_ax, 0, dt2*noise_ax, 0,
                 0, dt3/2*noise_ay, 0, dt2*noise_ay;
    }

  ekf_.Predict();

  /*****************************************************************************
   *  Update
   ****************************************************************************/

  /**
     * Use the sensor type to perform the update step.
     * Update the state and covariance matrices.
   */

  if (measurement_pack.sensor_type_ == MeasurementPackage::RADAR) {
    // Radar updates
    // prediction is in the x_ attribute of the filter
    Hj_ << tools.CalculateJacobian(ekf_.x_);
    ekf_.H_ = Hj_;
    ekf_.R_ = R_radar_;
    ekf_.UpdateEKF(measurement_pack.raw_measurements_);
  } else {
    // Laser updates
    ekf_.H_ = H_laser_;
    ekf_.R_ = R_laser_;
    ekf_.Update(measurement_pack.raw_measurements_);
  }

  // print the output
  cout << "x_ = " << ekf_.x_ << endl;
  cout << "P_ = " << ekf_.P_ << endl;
}
