## ---------------------------------------------------------------------------------------------------
## NRG Installation Optimization Example
## ---------------------------------------------------------------------------------------------------
## Created By:       Salvador J. Nunez G.
## Created Date:     2015-06-20
## ---------------------------------------------------------------------------------------------------

##--------------------------------------------------------------------------
## Load Packages
##--------------------------------------------------------------------------
library("simmer")
library("dplyr")


##--------------------------------------------------------------------------
## Set Summary 
##--------------------------------------------------------------------------

##--------------------------------------------------------------------------
## Simmer
##--------------------------------------------------------------------------

t0<-
  create_trajectory("my trajectory") %>%
  ## add an intake event 
  add_seize_event("nurse",1.0) %>%
  add_timeout_event(15) %>%
  add_release_event("nurse",1.0) %>%
  ## add a consultation event
  add_seize_event("doctor",1.0) %>%
  add_timeout_event(20) %>%
  add_release_event("doctor",1.0) %>%
  ## add a planning event
  add_seize_event("administration",1.0) %>%
  add_timeout_event(5) %>%
  add_release_event("administration",1.0)


t1<-
  create_trajectory("my trajectory") %>%
  ## add an intake event 
  add_seize_event("nurse",1.0) %>%
  add_timeout_event("rnorm(1,15)") %>%
  add_release_event("nurse",1.0) %>%
  ## add a consultation event
  add_seize_event("doctor",1.0) %>%
  add_timeout_event("rnorm(1,20)") %>%
  add_release_event("doctor",1.0) %>%
  ## add a planning event
  add_seize_event("administration",1.0) %>%
  add_timeout_event("rnorm(1,5)") %>%
  add_release_event("administration",1.0)


sim<-
  create_simulator("SuperDuperSim", n = 100, until = 80) %>%
  add_resource("nurse", r) %>%
  add_resource("doctor", r) %>%
  add_resource("administration", r)


sim<-
  sim %>%
  add_entities_with_interval(trajectory = t1, n = 10, name_prefix = "patient", interval =  "rnorm(1, 10, 2)")


sim<-
  sim %>%
  add_entity(trajectory = t1, name = "separate_patient" , activation_time =  100)


sim <-
  sim %>%
  simmer()

t2<-
  create_trajectory("trajectory with a skip event") %>%
  ## add a skip event - (50 - 50 chance that the next event is skipped)
  add_skip_event(number_to_skip = "sample(c(0,1),1)") %>%
  add_timeout_event(15) %>%
  add_timeout_event(5)


##--------------------------------------------------------------------------
## Record Results
##--------------------------------------------------------------------------

monitor_values = get_entity_monitor_values(sim, aggregated = TRUE)
x = sum(monitor_values$flow_time>=t)/sum(monitor_values$flow_time>=0)
u = plot_resource_utilization(sim, c("nurse", "doctor","administration"))
util = mean(u$data$Q50)

