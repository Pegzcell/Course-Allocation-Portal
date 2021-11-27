#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define br printf("\n")
#define fo(i, n) for (int i = 0; i < n; i++)
#define Fo(i, k, n) for (int i = k; k < n ? i < n : i > n; k < n ? i += 1 : i -= 1)

#define TUT_TIME 2
#define SLOTS_FILLING 5
#define RELAX 3
#define LAST 10000

//Regular bold text
#define BBLK "\e[1;30m"
#define BRED "\e[1;31m"
#define BGRN "\e[1;32m"
#define BYEL "\e[1;33m"
#define BBLU "\e[1;34m"
#define BMAG "\e[1;35m"
#define BCYN "\e[1;36m"
#define ANSI_RESET "\x1b[0m"

typedef enum states{AVAILABLE, BUSY, EXHAUSTED} ta_status;

typedef struct ta_stats{
	int id;
	int times;
	ta_status status;
} ta_stats;

typedef struct course{
	int id;
	char name[20];
	float interest;
	int max_slots;
	int curr_slots;
	int no_labs;
	int *labs_list;
	int dropped;
} course;

typedef struct stu{
	int id;
	int enrolled;
	float calibre;
	int pref[3];
	int time;
	int curr_pref;
} student;

typedef struct lab{
	int id;
	char name[20];
	int no_ta;
	int max_times;
	ta_stats** ta_list;
	int valid;
} lab;

int s, l, c;
course **courses;
student **students;
lab **labss;

pthread_mutex_t prnt_lock;

#define pp pthread_mutex_lock(&prnt_lock);
#define ppo pthread_mutex_unlock(&prnt_lock);

sem_t *stud_sem;

pthread_mutex_t *c_locks;
pthread_cond_t *cccc;
int* flags;

pthread_mutex_t *t_locks;
pthread_cond_t *tttt;
int* tutes;

pthread_mutex_t *dropped_locks;
pthread_mutex_t *curr_slots_locks;

pthread_mutex_t *lab_locks;

int available(int lab_id){
	lab* TA_lab = labss[lab_id];
	ta_stats **TAs = TA_lab->ta_list;
	int num = TA_lab->no_ta;
	int min = LAST, min_id;
	int ex =0;
	fo(i, num){
		if(TAs[i]->status == EXHAUSTED){
			ex++;
			continue;
		}
		else if(TAs[i]->status == AVAILABLE && TAs[i]->times < min){
			min = TAs[i]->times;
			min_id = TAs[i]->id;
		}
	}
	if (ex == num){
		TA_lab->valid =0;
		return -1;
	}
	else if (min == LAST){
		return -1;
	}
	TAs[min_id]->status = BUSY;
	return min_id;
}

char* ordinal(int n){
	if (n%10 ==1){
		return "st";
	}
	else if (n%10 ==2){
		return "nd";
	}
	else if (n%10 ==3){
		return "rd";
	}
	else{
		return "th";
	}
}

int slot_alloc(int max_val){
	srand(time(0));
	int r = (rand() % max_val) + 1;
	return r;
}

int decide(float prob){
	srand(time(0));
	int d = (rand() % 100) > prob * 100 ? 0 : 1;
	return d;
}

void *thread_student(void *stu_deets){
	student *stud_deets = (student *)stu_deets;
	sleep(stud_deets->time);
	stud_deets->enrolled = 1;
	int c_pref, d;	
	pp;
	printf("Student %d has filled in preferences for course registration\n", stud_deets->id);
	ppo;
	//student enrolled
	c_pref = stud_deets->pref[stud_deets->curr_pref];
	while(stud_deets->curr_pref < 3){
		pthread_mutex_lock(&c_locks[c_pref]);
		while (flags[c_pref] == 0){
			pthread_cond_wait(&cccc[c_pref], &c_locks[c_pref]);
		}
		pthread_mutex_unlock(&c_locks[c_pref]);

		pthread_mutex_lock(&dropped_locks[c_pref]);
		d = courses[c_pref]->dropped;
		pthread_mutex_unlock(&dropped_locks[c_pref]);

		if(d){
			stud_deets->curr_pref++;
			c_pref = stud_deets->pref[stud_deets->curr_pref];
			if (stud_deets->curr_pref == 3) break;
			pp;
			printf(BYEL "Student %d has changed current preference to %s (priority %d) since %s (priority %d) was withdrawn\n" ANSI_RESET, stud_deets->id, courses[c_pref]->name, stud_deets->curr_pref + 1, courses[stud_deets->pref[stud_deets->curr_pref - 1]]->name, stud_deets->curr_pref);
			ppo;
			//preference of student changed since course dropped
			continue; 
		}

		if(sem_trywait(&stud_sem[c_pref]) == -1 || flags[c_pref] == 0){
			//student couldn't enroll because all slots are filled or slot filling closed
			flags[c_pref] = 0;
			continue;
		}
		pp;
		printf(BCYN "Student %d has been allocated a seat in course %s\n" ANSI_RESET, stud_deets->id, courses[c_pref]->name);
		ppo;
		//student is allocated seat
		pthread_mutex_lock(&curr_slots_locks[c_pref]);
		courses[c_pref]->curr_slots++;
		pthread_mutex_unlock(&curr_slots_locks[c_pref]);
		
		tutes[c_pref] =1;
		pthread_mutex_lock(&t_locks[c_pref]);	
		while (tutes[c_pref] == 1){
			pthread_cond_wait(&tttt[c_pref], &t_locks[c_pref]);
		}
		pthread_mutex_unlock(&t_locks[c_pref]);
		//selection time
		if(decide(stud_deets->calibre * courses[c_pref]->interest)){
			pp;
			printf(BGRN "Student %d has selected the course %s permanently\n" ANSI_RESET, stud_deets->id, courses[c_pref]->name);
			ppo;
			//course decided
			return NULL;
		}
		else{
			stud_deets->curr_pref++;
			c_pref = stud_deets->pref[stud_deets->curr_pref];
			if (stud_deets->curr_pref == 3) break;
			pp;
			printf(BYEL "Student %d has changed current preference from %s (priority %d) to %s (priority %d)\n" ANSI_RESET, stud_deets->id, courses[stud_deets->pref[stud_deets->curr_pref - 1]]->name, stud_deets->curr_pref, courses[c_pref]->name, stud_deets->curr_pref + 1);
			ppo;
			//preference of student changed
		}
	}
	pp;
	printf(BGRN "Student %d could not get any of their preferred courses\n" ANSI_RESET, stud_deets->id);
	ppo;
	//student didnt get any course
}

void *thread_course(void *cours_deets){
	course *course_deets = (course *)cours_deets;
	int TA, slots, curr_slots, c_id = course_deets->id, TA_lab_id, ship, check;
	lab* TA_lab;
	while(1){
		fo(i, course_deets->no_labs){
			check = 0;
			TA_lab_id = course_deets->labs_list[i];
			TA_lab = labss[TA_lab_id];
			pthread_mutex_lock(&lab_locks[TA_lab_id]);
			if (!TA_lab->valid){
				TA = -1;
				check++;
				pthread_mutex_unlock(&lab_locks[TA_lab_id]);
				continue;
			}
			TA = available(TA_lab_id);
			pthread_mutex_unlock(&lab_locks[TA_lab_id]);

			if (TA != -1){
				ship = (TA_lab->ta_list[TA]->times) + 1;
				break;
			}
		}
		if (check == course_deets->no_labs){
			pp;
			printf(BRED "Course %s has been withdrawn\n" ANSI_RESET, course_deets->name);
			ppo;
			//course dropped due to inavailablity of TAs
			pthread_mutex_lock(&dropped_locks[c_id]);
			course_deets->dropped = 1;
			pthread_mutex_unlock(&dropped_locks[c_id]);

			pthread_mutex_lock(&c_locks[c_id]);
			flags[c_id] = 1;
			pthread_cond_broadcast(&cccc[c_id]);
			pthread_mutex_unlock(&c_locks[c_id]);

			break;
		}
		else if(TA == -1){
			continue;
		}

		pp;
		printf(BCYN "TA %d from lab %s has been allocated to course %s for their %d%s TA ship\n" ANSI_RESET, TA, TA_lab->name, course_deets->name, ship, ordinal(ship));
		ppo;
		//ta decided
		slots = slot_alloc(course_deets->max_slots);
		pp;
		printf(BBLU "Course %s has been allocated %d seat%c\n" ANSI_RESET, course_deets->name, slots, slots == 1?'\0':'s');
		ppo;
		//slots alloc
		sem_init(&(stud_sem[c_id]), 0, slots);
		
		pthread_mutex_lock(&c_locks[c_id]);
		flags[c_id] = 1;
		pthread_cond_broadcast(&cccc[c_id]);
		pthread_mutex_unlock(&c_locks[c_id]);
		//filling slots
		sleep(SLOTS_FILLING);

		//slot filling ends
		flags[c_id]=0;

		pthread_mutex_lock(&curr_slots_locks[c_id]);
		curr_slots = courses[c_id]->curr_slots;
		courses[c_id]->curr_slots = 0;
		pthread_mutex_unlock(&curr_slots_locks[c_id]);
		pp;
		printf(BBLU "Tutorial has started for Course %s with %d seat%c filled out of %d\n" ANSI_RESET, course_deets->name, curr_slots, curr_slots == 1?'\0':'s', slots);
		ppo;
		//tut begins with curr_slots
		sleep(TUT_TIME);
		pp;
		printf(BMAG "TA %d from lab %s has completed the tutorial for course %s\n" ANSI_RESET, TA, TA_lab->name, course_deets->name);
		ppo;
		//tut ends
		sem_destroy(&stud_sem[c_id]);

		pthread_mutex_lock(&t_locks[c_id]);
		tutes[c_id] = 0;
		pthread_cond_broadcast(&tttt[c_id]);
		pthread_mutex_unlock(&t_locks[c_id]);

		pthread_mutex_lock(&lab_locks[TA_lab_id]);
		TA_lab->ta_list[TA]->status = (TA_lab->max_times == ++(TA_lab->ta_list[TA]->times)) ? EXHAUSTED : AVAILABLE;
		pthread_mutex_unlock(&lab_locks[TA_lab_id]);
		sleep(RELAX);
	}
	return NULL;
}

int main(){
	//////////////////////////////////////////INPUT
	int num;
	scanf("%d %d %d", &s, &l, &c);

	courses = (course**)calloc(c, sizeof(course*));
	students = (student**)calloc(s, sizeof(student*));
	labss = (lab**)calloc(l, sizeof(lab*));

	fo(i,c){
		courses[i] = (course*)malloc(sizeof(course));
		courses[i]->id = i;		
		scanf("%s %f %d %d", courses[i]->name, &(courses[i]->interest), &(courses[i]->max_slots), &num);
		courses[i]->no_labs = num;
		courses[i]->labs_list = (int*)calloc(num, sizeof(int));
		fo (j, num){
			scanf("%d", &(courses[i]->labs_list[j]));
		}
		courses[i]->dropped = 0;
		courses[i]->curr_slots = 0;
	}

	fo(i,s){
		students[i] = (student*)malloc(sizeof(student));
		students[i]->id = i;
		scanf("%f %d %d %d %d", &(students[i]->calibre), &(students[i]->pref[0]), &(students[i]->pref[1]), &(students[i]->pref[2]), &(students[i]->time));
		students[i]->curr_pref = 0;
		students[i]->enrolled = 0;
		//printf(" (%d %d %f) ",students[i]->id, students[i]->time, students[i]->calibre);
	}

	fo(i,l){
		labss[i] = (lab*)malloc(sizeof(lab));
		labss[i]->id = i;
		scanf("%s %d %d", labss[i]->name, &num, &(labss[i]->max_times));
		labss[i]->no_ta = num;
		labss[i]->ta_list = (ta_stats**)calloc(num, sizeof(ta_stats*));
		fo(j, num){
			labss[i]->ta_list[j] = (ta_stats*)malloc(sizeof(ta_stats));
			labss[i]->ta_list[j]->id = j;
			labss[i]->ta_list[j]->times = 0;
			labss[i]->ta_list[j]->status = AVAILABLE;
		}
		labss[i]->valid =1;
	}
	/////////////////////////////////////////LOCKS, SEMAPHORES, COND_VARIABLES
	pthread_mutex_init(&prnt_lock, NULL);

	stud_sem = (sem_t *)calloc(c, sizeof(sem_t));

	c_locks = (pthread_mutex_t*)calloc(c, sizeof(pthread_mutex_t));
	cccc = (pthread_cond_t*)calloc(c, sizeof(pthread_cond_t));
	flags = (int*)calloc(c, sizeof(int));

	t_locks = (pthread_mutex_t*)calloc(c, sizeof(pthread_mutex_t));
	tttt = (pthread_cond_t*)calloc(c, sizeof(pthread_cond_t));
	tutes = (int*)calloc(c, sizeof(int));

	dropped_locks = (pthread_mutex_t*)calloc(c, sizeof(pthread_mutex_t));
	curr_slots_locks = (pthread_mutex_t*)calloc(c, sizeof(pthread_mutex_t));

	fo(i, c){
		pthread_mutex_init(&c_locks[i], NULL);
		pthread_cond_init(&cccc[i], NULL);
		pthread_mutex_init(&t_locks[i], NULL);
		pthread_cond_init(&tttt[i], NULL);
		pthread_mutex_init(&dropped_locks[i], NULL);
		pthread_mutex_init(&curr_slots_locks[i], NULL);
	}

	lab_locks = (pthread_mutex_t*)calloc(l, sizeof(pthread_mutex_t));
	
	fo(i, l){
		pthread_mutex_init(&lab_locks[i], NULL);
	}
	////////////////////////////////////////THREADs
	pthread_t course_thread_ids_arr[c];

    fo(i,c)
    {
        pthread_create(&course_thread_ids_arr[i], NULL, thread_course, (void *)(courses[i]));
    }

    pthread_t student_thread_ids_arr[s];
    
    fo(i,s){
        pthread_create(&student_thread_ids_arr[i], NULL, thread_student, (void *)(students[i]));
	}

	//// run main till all threads close
	fo(i,s)
    {
        pthread_join(student_thread_ids_arr[i], NULL);
    }
/*
   	fo(i,c)
    {
        pthread_join(course_thread_ids_arr[i], NULL);
    }
*/
	///////////////////////////////////////destroyLOCKS
	pthread_mutex_destroy(&prnt_lock);

	fo(i, c){
		pthread_mutex_destroy(&c_locks[i]);
		pthread_cond_destroy(&cccc[i]);
		pthread_mutex_destroy(&t_locks[i]);
		pthread_cond_destroy(&tttt[i]);
		pthread_mutex_destroy(&dropped_locks[i]);
		pthread_mutex_destroy(&curr_slots_locks[i]);
	}
	
	fo(i, l){
		pthread_mutex_destroy(&lab_locks[i]);
	}

	br;
}