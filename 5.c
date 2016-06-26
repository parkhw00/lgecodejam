
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <limits.h>

#if 1
#define THREADS	4
#define debug(f,a...)	do{}while(0)
#else
#define DEBUG
#define THREADS	1
#define debug(f,a...)	printf("%16s.%-4d."f,__func__, __LINE__,##a)
#endif
#define error(f,a...)	printf("%16s.%-4d."f,__func__, __LINE__,##a)


#define list_next(j,l)			(j->city_list[(l)->number].next)
#define list_prev(j,l)			(j->city_list[(l)->number].prev)
#define list_init(j,l)			{list_next(j,l) = list_prev(j,l) = (l);}
#define list_add_before(j,to,new)	{ \
	list_next(j,list_prev(j,to)) = (new); \
	list_prev(j,new) = list_prev(j,to); \
	list_prev(j,to) = (new); \
	list_next(j,new) = (to); \
}
#define list_remove(j, l)	{ \
	list_next(j,list_prev(j,l)) = list_next(j,l); \
	list_prev(j,list_next(j,l)) = list_prev(j,l); \
	list_next(j,l) = list_prev(j,l) = NULL; \
}

struct jobcontrol;

struct jobarg
{
	int number;
	int running;
	struct jobcontrol *jc;
	unsigned long long result;

	int N;

	int *P, *S;
};

struct jobcontrol
{
	FILE *in, *out;

	pthread_cond_t jobdone;
	pthread_mutex_t lock;

	int next_job;
	struct jobarg *arg;
	pthread_t *tid;
	int T;
};

int max_power (struct jobarg *j, int size, int *P, int *S
#ifdef DEBUG
		, int debug_start
#endif
		)
{
	int first;
	int max = 0;

#ifdef DEBUG
	debug_start %= j->N;
	{
		int i;
		char buf[size*5+1];
		int off;

		off = 0;
		for (i=0; i<size; i++)
			off += sprintf (buf+off, " %4d", P[i]);

		debug ("%4d-%4d power %*s%s\n",
				debug_start, debug_start + size,
				5*debug_start, "", buf);

		off = 0;
		for (i=0; i<size; i++)
			off += sprintf (buf+off, " %4d", S[i]);

		debug ("%4d-%4d sex   %*s%s\n",
				debug_start, debug_start + size,
				5*debug_start, "", buf);
	}
#endif

	for (first=0; first<size-1; first++)
	{
		int second;

		for (second=first+1; second<size; second++)
		{
			int diff_sex;
			int subAsize, subBsize;
			int maxA, maxB;
			int power;

			debug ("%4d-%4d check %d and %d\n",
					debug_start, debug_start + size,
					debug_start + first,
					debug_start + second);

			diff_sex = S[first] ^ S[second];
			if (!diff_sex)
				continue;

			subAsize = second-first-1;
			subBsize = size - subAsize - 2;
			debug ("%4d-%4d sub section size A %d, B %d\n",
					debug_start, debug_start + size,
					subAsize, subBsize);

			maxA = maxB = 0;
			if (subAsize > 1)
				maxA = max_power (j, subAsize, P+first+1, S+first+1
#ifdef DEBUG
						, debug_start + first + 1
#endif
						);

			if (subBsize > 1)
			{
				int tailsize;

				tailsize = size-first-subAsize-2;

				if (first == 0)
				{
					maxB = max_power (j, subBsize, P + second + 1, S + second + 1
#ifdef DEBUG
							, debug_start + second + 1
#endif
							);
				}
				else if (tailsize == 0)
				{
					maxB = max_power (j, subBsize, P, S
#ifdef DEBUG
							, 0
#endif
							);
				}
				else
				{
					int *subP, *subS;

					subP = malloc (subBsize*sizeof (subP[0]));
					subS = malloc (subBsize*sizeof (subS[0]));

					memcpy (subP, P+first+subAsize+2, tailsize*sizeof(subP[0]));
					memcpy (subS, S+first+subAsize+2, tailsize*sizeof(subS[0]));
					memcpy (subP+tailsize, P, first*sizeof(subP[0]));
					memcpy (subS+tailsize, S, first*sizeof(subS[0]));

					maxB = max_power (j, subBsize, subP, subS
#ifdef DEBUG
							, debug_start + second + 1
#endif
							);

					free (subP);
					free (subS);
				}
			}

			power = P[first]*P[second] + maxA + maxB;
			if (power > max)
				max = power;
		}
	}

	debug ("%4d-%4d max %d\n",
			debug_start, debug_start + size,
			max);

	return max;
}

void * jobthread (void *arg)
{
	struct jobarg *j = arg;

	/* do job */
	j->result = max_power (j, j->N, j->P, j->S
#ifdef DEBUG
			, 0
#endif
			);

	/* mark we done */
	pthread_mutex_lock (&j->jc->lock);
	j->running = 0;
	printf ("%d. %llu\n", j->number, j->result);
	pthread_mutex_unlock (&j->jc->lock);
	pthread_cond_signal (&j->jc->jobdone);

	/* cleanup not used memory */
	free (j->P);
	free (j->S);

	return 0;
}

int more_job (struct jobcontrol *jc)
{
	struct jobarg *j;
	int jn;

	int a;

	if (jc->next_job >= jc->T)
		return 0;
	jn = jc->next_job ++;

	j = jc->arg + jn;

	fscanf (jc->in, "%d\n", &j->N);
	printf ("%d. new job. N %d\n", jn, j->N);

	j->P = calloc (j->N, sizeof (j->P[0]));
	for (a=0; a<j->N; a++)
	{
		int p;

		p = 0;
		fscanf (jc->in, "%d", &p);

		j->P[a] = p;
	}


	j->S = calloc (j->N, sizeof (j->S[0]));
	for (a=0; a<j->N; a++)
	{
		int s;

		s = 0;
		fscanf (jc->in, "%d", &s);

		j->S[a] = s;
	}

	/* set jobthread argument and run the thread */
	j->number = jn;
	j->running = 1;
	j->jc = jc;

	pthread_create (jc->tid+jn, NULL, jobthread, jc->arg+jn);
	pthread_detach (jc->tid[jn]);

	return 0;
}

int now_running (struct jobcontrol *jc)
{
	int i;
	int running = 0;

	for (i=0; i<jc->T; i++)
		if (jc->arg[i].running)
			running ++;

	return running;
}

int main (int argc, char **argv)
{
	char *_in, *_out, *p;
	struct jobcontrol _jc = {0,};
	struct jobcontrol *jc = &_jc;
	int i;

	if (argc < 2)
	{
		error ("no input\n");
		return 1;
	}

	_in = argv[1];
	_out = malloc (strlen (_in) + 5);
	strcpy (_out, _in);
	p = strrchr (_out, '.');
	if (!p)
		p = _out+strlen (_in);
	strcpy (p, ".out");

	debug ("in %s, out %s\n", _in, _out);
	jc->in = fopen (_in, "r");
	jc->out = fopen (_out, "w");
	if (!jc->in || !jc->out)
	{
		error ("no input or output\n");
		return 1;
	}

	pthread_mutex_init (&jc->lock, NULL);
	pthread_cond_init (&jc->jobdone, NULL);

	fscanf (jc->in, "%d\n", &jc->T);
	printf ("T %d\n", jc->T);

	jc->arg = calloc (jc->T, sizeof (jc->arg[0]));
	jc->tid = calloc (jc->T, sizeof (jc->tid[0]));
	if (!jc->arg || !jc->tid)
	{
		error ("no mem for arg or tid\n");
		return 1;
	}

	while (1)
	{
		int running;

		pthread_mutex_lock (&jc->lock);
		running = now_running (jc);
		if (running >= THREADS)
			pthread_cond_wait (&jc->jobdone, &jc->lock);
		else if (jc->next_job >= jc->T)
		{
			if (running == 0)
			{
				debug ("all done.\n");
				pthread_mutex_unlock (&jc->lock);
				break;
			}

			pthread_cond_wait (&jc->jobdone, &jc->lock);
		}
		pthread_mutex_unlock (&jc->lock);

		more_job (jc);
	}

	for (i=0; i<jc->T; i++)
	{
		printf ("%d. result %llu\n", i, jc->arg[i].result);
		fprintf (jc->out, "%llu\n", jc->arg[i].result);
	}

	return 0;
}

