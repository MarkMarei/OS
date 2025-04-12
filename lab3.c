#include <pthread.h>
#include "caltrain.h"
// pthread_cond_wait pthread_cond_signal pthread_cond_proadcast

void station_init(struct station *station)
{
	pthread_mutex_init(&station->lock, NULL);
    pthread_cond_init(&station->condition_full, NULL);
    pthread_cond_init(&station->condition_Train_arrive, NULL);

    station->NumberOfEmptySeats = 0;
    station->NumberOfPassengersWalkingOnTrain = 0;
    station->NumberOfWaitingPassengers = 0;
}

void station_load_train(struct station *station, int count)
{
    pthread_mutex_lock(&(station->lock));
	station->NumberOfEmptySeats = count;
    pthread_cond_broadcast(&(station->condition_Train_arrive));

    // wait until last passenger enters and sets || all sets are complete and people 2a3dt
    while((station->NumberOfEmptySeats > 0 && station->NumberOfWaitingPassengers > 0) || 
        (station->NumberOfPassengersWalkingOnTrain > 0))
	{
		pthread_cond_wait(&(station->condition_full) ,&(station->lock));
	}
    station->NumberOfEmptySeats = 0;
    pthread_mutex_unlock(&(station->lock));
}

void station_wait_for_train(struct station *station)
{
    // wait until a train with at least one empty seat exist
    pthread_mutex_lock(&(station->lock));
	station->NumberOfWaitingPassengers++;
    while(station->NumberOfEmptySeats == 0)
	{
		pthread_cond_wait(&(station->condition_Train_arrive) ,&(station->lock));
	}
    station->NumberOfWaitingPassengers--;
    station->NumberOfEmptySeats--;
    station->NumberOfPassengersWalkingOnTrain++;

    pthread_mutex_unlock(&(station->lock));

}


void station_on_board(struct station *station)
{
    pthread_mutex_lock(&(station->lock));
	station->NumberOfPassengersWalkingOnTrain--;
    // if train is full let the train move
    if(station->NumberOfPassengersWalkingOnTrain == 0){
        pthread_cond_broadcast(&(station->condition_full));
    }
    pthread_mutex_unlock(&(station->lock));
}
