
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


struct jobcontrol;

struct jobarg
{
	int number;
	int running;
	struct jobcontrol *jc;
	unsigned long long result;

	int N;

	int *P, *W;
	int pos_w[50];
	int count_w;
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

int women_count (struct jobarg *j,
		int start, int count,
		int *W, int *rpos)
{
	int i;
	int ret;

	ret = 0;
	for (i=0; i<count; i++)
	{
		int pos;

		if (!W[i])
			continue;

		rpos[ret] = i;
		ret ++;
	}

	return ret;
}

int get_power (struct jobarg *j, int start, int count,
		int first, int second, int *P, int *W)
{
	int subAsize, subBsize;
	int maxA, maxB;
	int power;

	subAsize = second-first-1;
	subBsize = count - subAsize - 2;
	debug ("%4d-%4d check %d and %d, sub section count A %d, B %d\n",
			start, start + count,
			start + first,
			start + second,
			subAsize, subBsize);

	maxA = maxB = 0;
	if (subAsize > 1)
		maxA = max_power (j, start + first + 1, subAsize,
				P+first+1, W+first+1);

	if (subBsize > 1)
	{
		int tailsize;

		tailsize = count-first-subAsize-2;

		if (first == 0)
		{
			maxB = max_power (j, start + second + 1, subBsize,
					P + second + 1, W + second + 1);
		}
		else if (tailsize == 0)
		{
			maxB = max_power (j, start, subBsize, P, W);
		}
		else
		{
			int *subP, *subW;

			subP = malloc (subBsize*sizeof (subP[0]));
			subW = malloc (subBsize*sizeof (subW[0]));

			memcpy (subP, P+first+subAsize+2, tailsize*sizeof(subP[0]));
			memcpy (subW, W+first+subAsize+2, tailsize*sizeof(subW[0]));
			memcpy (subP+tailsize, P, first*sizeof(subP[0]));
			memcpy (subW+tailsize, W, first*sizeof(subW[0]));

			maxB = max_power (j, start + second + 1, subBsize, subP, subW);

			free (subP);
			free (subW);
		}
	}

	return P[first]*P[second] + maxA + maxB;
}

int max_power (struct jobarg *j, int start, int count, int *P, int *W)
{
	int first;
	int i;
	int max = 0;
	int w_pos[50];
	int w_count;

	w_count = women_count (j, start, count, W, w_pos);
	if (w_count == 0)
	{
		debug ("%4d-%4d no women\n",
				start, start + count);
		return 0;
	}
	if (w_count == count)
	{
		debug ("%4d-%4d no man\n",
				start, start + count);
		return 0;
	}

#ifdef DEBUG
	{
		char buf[count*5+1];
		int off;

		off = 0;
		for (i=0; i<count; i++)
			off += sprintf (buf+off, " %4d", i+start);

		debug ("%4d-%4d number%*s%s\n",
				start, start + count,
				5*start, "", buf);

		off = 0;
		for (i=0; i<count; i++)
			off += sprintf (buf+off, " %4d", P[i]);

		debug ("%4d-%4d power %*s%s\n",
				start, start + count,
				5*start, "", buf);

		off = 0;
		for (i=0; i<count; i++)
			off += sprintf (buf+off, " %4d", W[i]);

		debug ("%4d-%4d woman %*s%s\n",
				start, start + count,
				5*start, "", buf);
	}
#endif

	for (i=0; i<w_count; i++)
	{
		int second;

		first = w_pos[i];

		for (second=0; second<count; second++)
		{
			int power;
			int f, s;

			if (W[second])
				continue;

			if (first > second)
			{
				f = second;
				s = first;
			}
			else
			{
				f = first;
				s = second;
			}

			power = get_power (j, start, count, f, s, P, W);
			if (power > max)
				max = power;
		}
	}

	debug ("%4d-%4d max %d\n",
			start, start + count,
			max);

	return max;
}

void * jobthread (void *arg)
{
	struct jobarg *j = arg;

	/* do job */
	j->result = max_power (j, 0, j->N, j->P, j->W);

	/* mark we done */
	pthread_mutex_lock (&j->jc->lock);
	j->running = 0;
	printf ("%d. %llu\n", j->number, j->result);
	pthread_mutex_unlock (&j->jc->lock);
	pthread_cond_signal (&j->jc->jobdone);

	/* cleanup not used memory */
	free (j->P);
	free (j->W);

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


	j->W = calloc (j->N, sizeof (j->W[0]));
	for (a=0; a<j->N; a++)
	{
		int s;

		s = 0;
		fscanf (jc->in, "%d", &s);

		j->W[a] = !!s;
		if (!!s)
		{
			j->pos_w[j->count_w] = a;
			j->count_w ++;
		}
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

