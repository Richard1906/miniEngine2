  /*

    Author: Airic Lenz
    Year of release: 2016
    
    See www.airiclenz.com for more information
    
    The 1st version of this code, dealing with core functionalities, 
    was heavily inspired by the OpenMoCo Engine by C.A. Church
    and is basically based on it. Thank you for your great work! 
        
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/



////////////////////////////////////////////////////////
//                                                    //
//  C O R E   V A R I A B L E S                       //
//                                                    //
////////////////////////////////////////////////////////


// B0 = program is running
// B1 = program just started
// B2 = jog mode
// B3 =  
// B4 = 
// B5 = 
// B6 = 
// B7 = 
uint8_t core_status;



// B0 = move motor to home pos before program start
// B1 = autosave settings
// B2 = loop
// B3 = looped-move flag (is set if the move is backwards)
// B4 = continue to shoot after done
// B5 = 
// B6 = 
// B7 = 
uint8_t core_settings = B00000011;






// ============================================================================
boolean core_isProgramRunningFlag()             { return isBit(core_status, BIT_0); }
void    core_setProgramRunningFlag()            { setBit(core_status, BIT_0); }
void    core_deleteProgramRunningFlag()         { deleteBit(core_status, BIT_0); }

boolean core_isProgramStartedFlag()             { return isBit(core_status, BIT_1); }
void    core_setProgramStartedFlag()            { setBit(core_status, BIT_1); }
void    core_deleteProgramStartedFlag()         { deleteBit(core_status, BIT_1); }

boolean core_isJogModeFlag()                    { return isBit(core_status, BIT_2); }
void    core_setJogModeFlag()                   { setBit(core_status, BIT_2); }
void    core_deleteJogModeFlag()                { deleteBit(core_status, BIT_2); }

boolean core_isMoveToHomeBeforeStartFlag()      { return isBit(core_settings, BIT_0); }
void    core_setMoveToHomeBeforeStartFlag()     { setBit(core_settings, BIT_0); }
void    core_deleteMoveToHomeBeforeStartFlag()  { deleteBit(core_settings, BIT_0); }
void    core_toggleMoveToHomeBeforeStartFlag()  { toggleBit(core_settings, BIT_0); }

boolean core_isLoopFlag()                       { return isBit(core_settings, BIT_2); }
void    core_setLoopFlag()                      { setBit(core_settings, BIT_2); }
void    core_deleteLoopFlag()                   { deleteBit(core_settings, BIT_2); }
void    core_toggleLoopFlag()                   { toggleBit(core_settings, BIT_2); } 

boolean core_isLoopedMoveFlag()                 { return isBit(core_settings, BIT_3); } 
void    core_setLoopedMoveFlag()                { setBit(core_settings, BIT_3); }
void    core_deleteLoopedMoveFlag()             { deleteBit(core_settings, BIT_3); }
void    core_toggleLoopedMoveFlag()             { toggleBit(core_settings, BIT_3); } 

boolean core_isEndlessShootFlag()              { return isBit(core_settings, BIT_4); } 
void    core_setEndlessShootFlag()             { setBit(core_settings, BIT_4); }
void    core_deleteEndlessShootFlag()          { deleteBit(core_settings, BIT_4); }
void    core_toggleEndlessShootFlag()          { toggleBit(core_settings, BIT_4); } 



// ============================================================================
// initialize the core parts of the software
// ============================================================================
void core_init() {
  
  // no status flags set so far
  core_status = 0;
    
}




// ============================================================================
// starts the program
// ============================================================================
bool core_startProgram() {
  
  // if the program is not yet running
  if (!core_isProgramRunningFlag()) {
    
    //////////////////////////////////////////////
    // P R O G R A M   I N I T I A L I Z A T I O N 
    ////////////////////////////////////////////// 
    
    // check the modes
    core_checkModes();
    // possibly do a recalculation of the core values
    core_checkValues();
    // reset the camera shoot count
    cam_resetShootCount();
    // set the program-is-running-flag
    core_setProgramRunningFlag();   
    // repaint the user interface
    uicore_setRepaintFlag();
    uicore_repaint(true);
    // turn all trigger interrupts on
    trigger_enableAllInterrupts();
    // remove the flag that indicated a 
    // return move in bouncing mode 
    core_deleteLoopedMoveFlag();
    // delete the start flag that might be set
    com_deletePrepareFlag();
    // delete the com stop flag that might be set
    com_deleteStopFlag();
    // delete the com sync flag that might be set
    com_deleteSyncFlag();

    
    //////////////////////////////////////////////
    // S T A R T   T R I G G E R S
    //////////////////////////////////////////////
        
    // clear the trigger cache for making sure we have not false
    // trigger events caused by enabling the interrupts
    trigger_clearEvents();  
    
    // check and enable triggers if we are no registered
    // slave device in the daisy chain
    if (trigger_isStartTriggerDefined() &&
        !com_isRegisteredSlaveFlag()) {
      
      // print a "waiting for trigger" message and show it for
      // at least one millisecond
      uicore_showMessage(220, 221, 222, 1);
  
      // wait until the trigger signal comes
      while (!trigger_isStartTriggered()) {
        
        // check the keys for abort events or
        // if the stop trigger came before the
        // start
        if (input_isKeyEvent() ||
            trigger_isStopTriggered()) {
          
          // clear the key input event buffer
          input_clearKeyEvent();
          // stop the program
          core_stopProgram(com.isMaster() && (com.getSlaveCount() > 0));
          // remove all possible meessages from the screen
          uicore_deleteMessageOnScreenFlag();
          // do a full repaint to have a fresh screen
          uicore_repaint(true);
          // remove the repaint flag
          uicore_deleteRepaintFlag();
                  
          // and now leave this function
          return false;
        }
          
      }
          
      // trigger the backlight on the trigger event
      uicore_setBacklight(true);
            
    }
   
    
    //////////////////////////////////////////////
    // S L A V E   P R E P A R A T I O N
    //////////////////////////////////////////////

    // if we are a master with registered slave devices
    if (com.isMaster() &&
        com.getSlaveCount() > 0) {
      
      // remove the ready flag for all slave devices    
      com_clearSlavesReady();    
      // send the prepare command to all slaves
      // this will be acknowledged from the slave devices
      // with a DONE signal so that we know when they 
      // finished their preparations
      com.sendCommand(MOCOM_BROADCAST, MOCOM_COMMAND_PREPARE);             
    }

    
    //////////////////////////////////////////////
    // M O T O R   P R E P A R A T I O N
    //////////////////////////////////////////////
    
    // print a "moving motor to home" message
    uicore_showMessage(225, 226, 226, 1);
    // enable the motors
    motor_powerAll();  
    // check if the motors need to be moved home first and
    // move them home in case "yes"
    core_checkMoveHomeBeforeStart();
    // store the current motor pos as start reference
    motor_storeMotorReferencePositions();
    // do we have curves assigned?
    motor_checkIfCurveExists();
    // define / check the moves we need to do
    motor_checkKeyframes();    
    
    
    // after the motor preparations are done, we need to take care
    // of some additional daisy chaining stuff:    
    // if we are a master with registered slave devices
    if (com.isMaster() &&
        com.getSlaveCount() > 0) {
          
      // check if the slaves are ready as well
      core_checkSlavePrepareStatus();
      // wait a little bit to allow everything to settle down after
      // the motors were avtivated
      core_delay(SYSTEM_START_DELAY);
      // send the go signal
      com.sendCommand(MOCOM_BROADCAST, MOCOM_COMMAND_SYNC);  
      
    } 

    
    // if we are a registered slave device
    else if (com_isRegisteredSlaveFlag()) {
      // print a message - waiting for master sync
      uicore_showMessage(233, 237, 238, 1);
      // send a done command and wait for acknowledge of the master
      com_sendDoneWithAck();
      
      // wait until the master sends the go signal
      // or until a key is pressed
      while (
             (!com_isSyncFlag()) &&
             (!input_isKeyEvent())
            )       
      {
        com.executeCommunication();  
      }
      
      // delete the received sync flag
      com_deleteSyncFlag();
      // delete the key flag in case a key was pressed
      input_clearKeyEvent();
    }
    
    // we are just a standalone device
    else {
      // wait a little bit to allow everything to settle down after
      // the motors were avtivated
      core_delay(SYSTEM_START_DELAY);

    }
    
        
    //////////////////////////////////////////////
    // R E P A I N T
    //////////////////////////////////////////////
        
    // remove all possible messages from the screen
    uicore_deleteMessageOnScreenFlag();
    // do a full repaint
    uicore_repaint(true);
    
        
    //////////////////////////////////////////////
    // S T A R T
    //////////////////////////////////////////////
    
    // if we are in one of the following modes,
    // start the curve based move for all motors
    if (isBit(core_mode, MODE_TIMELAPSE) || 
        isBit(core_mode, MODE_VIDEO)) {

            
      if (isBit(core_move_style, MOVE_STYLE_CONTINUOUS)) {
        
        // check the max speeds...
        if (!motor_checkCurvesMaxSpeedOk()) {

          // stop everything
          core_stopProgram(com.isMaster() && (com.getSlaveCount() > 0));
          // do a full repaint to have a fresh screen
          // and updated variables for not painting
          // the screen unneededly
          uicore_repaint(true);
          // remove the repaint flag
          uicore_deleteRepaintFlag();
          // send a "max speed exceeded" message to the user
          uicore_showMessage(227, 228, 229, 3000);
          // leave this function
          return false;
          
        }
        
        // start a continuous move if we are in continuous mode
        motor_startContinuousMove();
      }
      
    }
    
    
    // set the flag indicating that we just started
    core_setProgramStartedFlag();
    
    
  } // end: program already running? 
  
  return true;
  
}


// ============================================================================
// this function is actually starting the program. It saves the start time
// and starts the motors
// ============================================================================
void core_checkIfStarted() {
  
  if (core_isProgramStartedFlag()) {
  
    // if we are a master send a 2nd start signal.
    // this 2nd signal is needed because now the clients are
    // checking the sync signal from the main loop and can react faster
    if (com.isMaster() && (com.getSlaveCount() > 0)) {
      
      // send the 2nd and final go signal
      com.sendCommand(MOCOM_BROADCAST, MOCOM_COMMAND_SYNC);  
      // wait until the clients are done receiving
      core_delay(30);
   
    } else if (com_isRegisteredSlaveFlag()) {
      
      // wait for the sync flag
      while (
               (!com_isSyncFlag()) &&
               (!input_isKeyEvent())
              )       
        {
          com.executeCommunication();  
        }
        
        // delete the received sync flag
        com_deleteSyncFlag();
    }
    
    // remember the time when we started  
    core_program_start_time = millis();
    // remember when the current cycle started
    system_cycle_start = millis();
    // start the timer...
    motor_startMoveTimer();  
    // delete the started-flag
    core_deleteProgramStartedFlag();
    
  }
  
}


// ============================================================================
// stops the program
// ============================================================================
void core_stopProgram(boolean all) {
  
  // if we are asked to stop all programs of all slaves, do it
  if (all && com.isMaster()) {
    com.sendCommand(MOCOM_BROADCAST, MOCOM_COMMAND_STOP);
  }
    
  // set the program-is-running-flag
  core_deleteProgramRunningFlag();   
 
  // remove the flag that indicated a 
  // return move in bouncing mode 
  core_deleteLoopedMoveFlag();
   
  // disable all motors on program Stop 
  motor_disableAll(); 
 
  // turn all trigger interrupts off
  trigger_disableAllInterrupts(); 
 
  // clear all remaining trigger events
  trigger_clearEvents();  
  
  // stop the camera as well
  cam_stop(); 

  // reset the cycle warning
  system_cycle_too_long = false;

  // reset the panorama stuff
  panorama_pos_m1 = 0;
  panorama_pos_m2 = 0;
 
  // enable the backlight 
  uicore_setBacklight(true); 
 
  // repaint the user interface
  uicore_setRepaintFlag();
  uicore_repaint(true);
 
    
}

// ============================================================================
// does a basic move of the saved curves
// ============================================================================
void core_doPreview() {
  
  // remeber the current move style
  byte old_style = core_move_style;
  
  // set the move style to continuous
  core_move_style = MOVE_STYLE_CONTINUOUS;

  // enable the motors
  motor_powerAll();  
  
  // start the moves
  motor_startContinuousMove();
  
  // start the motor timer...
  motor_startMoveTimer();

  
  // wait until the preview is done  
  while (
          (motor_isCurveBasedMoveRunning()) &&
          (! input_isKeyEvent())
        ) {
    
    motor_process();
    input_process();
            
  }
  
  // disable all motors on program Stop 
  motor_disableAll(); 
        
  // clear all possible input events;      
  input_clearKeyEvent();      
  
  // re-set the old mode
  core_move_style = old_style;
  
  
}



// ===================================================================================
// prepares all registered slaves for the program start
// ===================================================================================
void core_checkSlavePrepareStatus() {
    
  // if we have no slaves, leave immediately
  if (com.getSlaveCount() == 0) return;
    
  // our main done flag
  boolean done = false;
  // temp variables for storing the received data
  byte command, id;
      
  // loop until we are done
  while (!done) {
    
    // check if we received something from the slaves
    // --> gets processes in the com_handle_events()
    com.executeCommunication();
    
    done = true;
    
    // loop the whole array and check if we are ready
    // (reset the main flag to false if not)
    for (int i=0; i<com.getSlaveCount(); i++) {
      
      byte res = isBit(com_slaveStati[i], BIT_0);
           
      // if the bit is not set
      if (res == 0) {
        done = false;
      }
      
    } // end: loop all slave devices
    
  } // end: main loop
    
}




// ============================================================================
// next shot needed?
// ============================================================================
void core_checkIfCycleWasToLong() {
  
  // paint a warning if the cycles take too long
  // but only if we are in SMS mode
  if (((system_cycle_start + setup_interval_length) < millis()) &&
      (isBit(core_mode, MOVE_STYLE_SMS))) {
    
    system_cycle_too_long = true;
    
    uicore_setRepaintFlag();

  }
}


// ============================================================================
// next shot needed?
// ============================================================================
boolean core_isNextCycle() {
  
  // are we a master?
  if (com.isMaster() &&
      com.getSlaveCount() > 0) {
        
    // is the cycle over?
    if ((core_program_start_time + (setup_interval_length * (uint32_t) cam_getShootCount())) <= millis()) {
      
      // send the global sync signal to start everything everywhere :)
      com.sendCommand(MOCOM_BROADCAST, MOCOM_COMMAND_SYNC);  
      
      // make sure we trigger exactly when the master is triggering the camera;
      // this delay was measured with an oscilloscope and is caused by the different
      // code that is execute on the master devices vs. the slave devices
      core_delay(4);
      // wait another millisecond and a bit to make it perfect (+/-250 microseconds = 1/2000 sec)
      delayMicroseconds(1);
      
      // yes, the cycle is over
      return true;
      
    } 
    // no, the cycle is not over
    else return false;
    
  }
  // we are valid slave in a chain
  else if (com_isRegisteredSlaveFlag()) {
  
    if (com_isSyncFlag()) {
      com_deleteSyncFlag();
      return true;
    }
    else {
      return false;  
    }
    
  }
  // we are a standalone device
  else {
    return (core_program_start_time + (setup_interval_length * (uint32_t) cam_getShootCount())) <= millis();
  }
  
  return false;
}




// ============================================================================
void core_checkIfTimelapseDone() {

  // are we supposed to do an endless shoot?
  // is yes, just leave and do nothing...
  if (core_isEndlessShootFlag()) return;
  
  // did we reach the program end?
  else if (cam_getShootCount() >= setup_frame_count) {
          
    // stop the program
    core_stopProgram(com.isMaster() && (com.getSlaveCount() > 0));
          
    // reset the system phase flag and thus restart the cycle
    system_phase = 0;
    
  }  
  
}


// ============================================================================
// next shot needed?
// ============================================================================
boolean core_checkIfVideoDone() {

  return (core_program_start_time + (setup_record_time * motor_getTimeFactor())) <= millis();
    
}



// ============================================================================
// checks if the motors need to be moved home before start - 
// and moves the motors home if "yes"
// ============================================================================
boolean core_checkMoveHomeBeforeStart() {
    
  // are we requested to move the motor to home before starting?
  if (core_isMoveToHomeBeforeStartFlag()) {
    
    // move all motors home first
    for (int i=0; i<DEF_MOTOR_COUNT; i++) {
      motor_defineMoveToPosition(i, 0, true);  
    }
    
    // start the moves
    motor_startMovesToPosition();
            
    // wait until all motors reached home
    while (motor_isMoveToPositionRunning()) {
                
      // check if we received answer from the clients          
      if (com.isMaster()) {
        // do communication...
        com.executeCommunication();
      }
      
      
      // abort if the select key is pressed
      if (input_isKeyEvent()) {
        
        if (input_getPressedKey() == KEY_1) {
          
          input_clearKeyEvent(); 
          
          return false;  
        }
   
        input_clearKeyEvent();   
        
      } // end: key event
      
    } // end: while motors are moving 
    
  } // end: move home first  
  
  return true;
  
}





// ============================================================================
// checks the system modes for any invalid configurations
// ============================================================================
void core_checkModes() {

  // if we are in video mode
  if (isBit(core_mode, MODE_VIDEO)) {
    
    core_move_style = MOVE_STYLE_CONTINUOUS;
    
  }
    
}



// ============================================================================
// checks all the values for the different modes
// ============================================================================
void core_checkValues() {
  
  ////////////////////////////
  // T I M E L A P S E
  if (isBit(core_mode, MODE_TIMELAPSE)) {
      
    setup_frame_count     = cam_fps * (setup_play_time / 1000.0); 
    setup_interval_length = setup_record_time / (setup_frame_count - 1);
    
  } // end: timalpse
  
  
  
  ////////////////////////////  
  // V I D E O

  ////////////////////////////  
  // P A N O R A M A
 
  
}




// ============================================================================
void core_delay(unsigned int milliseconds) {
  
  unsigned long startTime = millis();

  while ((startTime + milliseconds) > millis()) {
    input_process();  
  } 
    
}


// ============================================================================
void core_deleteSettings() {
  
  // remove the old settings file
  sd_deleteSettings(); 
  // show the reset message
  uicore_showMessage(115, 116, 118, 1000);  

  // wait forever until the user removed the power
  while(1){};  
  
}


// ======================================================================================
// Function to restart the Arduino
// ======================================================================================
//core_reset() function:
void(* core_reset) (void) = 0; //declare reset function @ address 0


