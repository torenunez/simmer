#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <Rcpp.h>

#include <map>
#include <limits>
#include <algorithm>
#include <list>

using std::endl;


class Resource;
class Event;

// ?? unused
bool event_sort_func(Event* i,Event* j)
{
	return (i->early_start_time < j->early_start_time);
}

double get_next_step(std::vector<Event*> event_queue, double now, double until)
{
	double min_end_time = until;
	double min_early_start_time = until;

	for(std::vector<Event*>::iterator it = event_queue.begin(); it != event_queue.end(); ++it) {

		if((*it)-> end_time >= 0){
			if((*it)->end_time < min_end_time) 
				min_end_time = (*it)->end_time;
		} else {
			if(!(*it)->enqueued && (*it)-> early_start_time < min_early_start_time) 
				min_early_start_time = (*it)-> early_start_time;
		}
	}

	double min_value = std::min(min_end_time, min_early_start_time);

	return min_value;
}

class Simulator
{
public:
	std::string name;
	std::vector<Event*> event_queue;
	std::vector<Entity*> entity_vec;

	std::map<std::string, Resource*> resource_map;


	double until;
	bool verbose;
	double current_time;

	void add_entity(Entity* ent) {
		ent->set_simulator(this);
		entity_vec.push_back(ent);
		
	}


	void add_resource(std::string res_name, int capacity, int queue_size) {
		Resource* res_obj = new Resource(res_name, capacity, queue_size);
		resource_map[res_name] = res_obj;
	}

	
	Resource* get_resource(std::string res_name){
		try {
			return resource_map[res_name];
		} catch (...) {
			// not found
			throw std::runtime_error("resource '" + res_name + "' not found (typo?)");
		}
	};

	double now() {
		return current_time;
	};
	void run();
	

	Simulator(std::string sim_name, double until_time): name(sim_name), until(until_time), verbose(false), current_time(0) {}
	Simulator(std::string sim_name, bool verbose): name(sim_name), until(std::numeric_limits<double>::max()), verbose(verbose), current_time(0) {}
	Simulator(std::string sim_name, double until_time, bool verbose = false): name(sim_name), until(until_time), verbose(verbose), current_time(0) {}

	~Simulator() {

		while(!entity_vec.empty()) delete entity_vec.back(), entity_vec.pop_back();
		while(!event_queue.empty()) delete event_queue.back(), event_queue.pop_back();

		std::map<std::string, Resource*>::iterator itr = resource_map.begin();
		while (itr != resource_map.end()) {
			delete itr->second;
			itr++;
		}
		resource_map.clear();
	}
};


void Simulator::run()
{
	if(verbose) Rcpp::Rcout << "Starting simulation..." << endl;
	
	current_time = entity_vec[0]->activation_time;

	// initialize events
	for(std::vector <Entity*>::iterator it = entity_vec.begin(); it != entity_vec.end(); ++it) {
		Event* ev = (*it)->get_event();
		ev->early_start_time = (*it)->activation_time;
		event_queue.push_back(ev);
	}

	std::vector <int> events_to_delete;
	
	while(event_queue.size()>0) {

		unsigned int i = 0;
		while(i  < event_queue.size()) {

			Event* ev = event_queue[i];

			if(ev->end_time < 0 && ev->early_start_time <= current_time) { // activity hasn't started yet, try to start it
				bool started = ev->try_to_start(&current_time);
				if(verbose) Rcpp::Rcout << "Starting " << ev->type << "  <  " << ev->parent_entity->name;
				if(started) {
					if(verbose) Rcpp::Rcout << ".....started" << endl;
					
				} else {
					if(verbose) Rcpp::Rcout << ".....failed" << endl;
					if(ev->parent_entity->leave) {
						// save the index of the current event to be deleted from the queue
						events_to_delete.push_back(i);
					}
				}


			}
			// activity has finished
			// non-timed activities can finish just after starting
			if(ev->end_time >= 0 && current_time >= ev->end_time) {

				// call event specific stopping procedure
				if(verbose) Rcpp::Rcout << "Stopping" << ev->type << "  <  " << ev->parent_entity->name;
				if(verbose) Rcpp::Rcout << ".....stopped" << endl;

				ev->stop(&current_time);
				

				// check if there is a next event to start for the current entity,
				if(ev->parent_entity->entity_event_vec.size() > 0 ) {

					Event* next_ev = ev->parent_entity->get_event();
					next_ev->early_start_time = current_time;
					event_queue.push_back(next_ev);

				} 
				else { // entity is finished
					ev->parent_entity->monitor->record(current_time, -999);
					
				}

				// save the index of the current event to be deleted from the queue
				events_to_delete.push_back(i);

			}

			i++ ;
		}

		std::sort(events_to_delete.begin(), events_to_delete.end(), std::greater<int>());
		for(std::vector<int>::iterator it = events_to_delete.begin(); it != events_to_delete.end(); ++it) {
			delete event_queue[*it];
			
			// yes, not very efficient, but we need to keep the event_queue's order
			event_queue.erase(event_queue.begin()+(*it));
		}
		events_to_delete.clear();
		
		current_time = get_next_step(event_queue, current_time, until);
		if(current_time >= until) break;
		if(verbose) Rcpp::Rcout << "Current simulation time: " << current_time << std::endl;

	}
	if(verbose) Rcpp::Rcout << "Simulation ended..." << endl;

}

#endif
