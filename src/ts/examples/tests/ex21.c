
static char help[] = "Bouncing ball example to test TS event feature.\n";

/*
        u1_t = u2
        u2_t = -9.8
*/

#include <petscts.h>

typedef struct {
  PetscScalar    *fvalue;      /* value of event functions at the end of the step*/
  PetscScalar    *fvalue_prev; /* value of event function at start of the step */
  PetscReal       ptime;       /* time after step completion */
  PetscReal       ptime_prev;  /* time at step start */
  PetscErrorCode  (*monitor)(TS,PetscReal,Vec,PetscScalar*,PetscInt*,PetscBool*,void*);
  PetscBool      *terminate;   /* 1 -> Terminate time stepping, 0 -> continue */
  PetscInt       *direction; /* Zero crossing direction: 1 -> Going positive, -1 -> Going negative, 0 -> Both */ 
  PetscInt        nevents;
  void           *monitorcontext;
} EventCtx;

EventCtx *event;

#undef __FUNCT__
#define __FUNCT__ "EventMonitorSet"
PetscErrorCode EventMonitorSet(TS ts,PetscInt nevents,PetscErrorCode (*eventmonitor)(TS,PetscReal,Vec,PetscScalar*,PetscInt*,PetscBool*,void*),void *mectx)
{
  PetscErrorCode ierr;
  PetscReal      t;
  Vec            U;
  PetscInt       i;

  PetscFunctionBegin;
  ierr = PetscMalloc(sizeof(EventCtx),&event);CHKERRQ(ierr);
  ierr = PetscMalloc(nevents*sizeof(PetscScalar),&event->fvalue);CHKERRQ(ierr);
  ierr = PetscMalloc(nevents*sizeof(PetscScalar),&event->fvalue_prev);CHKERRQ(ierr);
  ierr = PetscMalloc(nevents*sizeof(PetscBool),&event->terminate);CHKERRQ(ierr);
  ierr = PetscMalloc(nevents*sizeof(PetscInt),&event->direction);CHKERRQ(ierr);
  event->monitor = eventmonitor;
  event->monitorcontext = (void*)mectx;
  event->nevents = nevents;

  ierr = TSGetTime(ts,&t);CHKERRQ(ierr);
  ierr = TSGetSolution(ts,&U);CHKERRQ(ierr);
  event->ptime_prev = t;
  ierr = (*event->monitor)(ts,t,U,event->fvalue_prev,NULL,NULL,NULL);CHKERRQ(ierr);

  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "EventFunction"
PetscErrorCode EventFunction(TS ts,PetscReal t,Vec U,PetscScalar *fvalue,PetscInt *direction,PetscBool *terminate,void *ctx)
{
  PetscErrorCode ierr;
  PetscScalar    *u;

  PetscFunctionBegin;
  ierr = VecGetArray(U,&u);CHKERRQ(ierr);
  fvalue[0] = u[0];
  if (terminate) terminate[0] = PETSC_TRUE;
  if (direction) direction[0] = -1;
  ierr = VecRestoreArray(U,&u);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "PostStep"
PetscErrorCode PostStep(TS ts)
{
  PetscErrorCode ierr;
  PetscReal      t;
  Vec            U;
  PetscInt       i;
  PetscReal      tstepstart,tstepend;
  PetscScalar    fvaluestart,fvalueend;

  PetscFunctionBegin;
  ierr = TSGetTime(ts,&t);CHKERRQ(ierr);
  ierr = TSGetSolution(ts,&U);CHKERRQ(ierr);

  tstepend = event->ptime = t;
  tstepstart = event->ptime_prev;

  ierr = (*event->monitor)(ts,t,U,event->fvalue,event->direction,event->terminate,NULL);CHKERRQ(ierr);
  for (i=0; i < event->nevents; i++) {

    fvaluestart = event->fvalue_prev[i];
    fvalueend   = event->fvalue[i];

    if (event->direction[i] < 0 && PetscSign(event->fvalue[i]) < 0 && PetscSign(event->fvalue_prev[i]) > 0) { /* Negative zero crossing */

      while (PetscAbs(event->fvalue[i]) > 1e-6) {
	if (PetscSign(event->fvalue[i]) == PetscSign(event->fvalue_prev[i])) {
	  event->fvalue_prev[i] = event->fvalue[i];
	  event->fvalue[i]      = fvalueend;
	  event->ptime_prev     = t;
	  event->ptime          = tstepend;
	} else {
	  event->fvalue_prev[i] = fvaluestart;
	  event->ptime =  t;
	  event->ptime_prev = tstepstart;
	  ierr = TSRollBack(ts);CHKERRQ(ierr);
	}
	/* Compute linearly interpolated new time step */
	PetscReal dt;
	PetscInt  stepnum;
	dt = -event->fvalue_prev[i]*(event->ptime - event->ptime_prev)/(event->fvalue[i] - event->fvalue_prev[i]);
	/* Set new time step */
	ierr = TSSetTimeStep(ts,dt);CHKERRQ(ierr);
	/* Take a step with the modified time step */
	ierr = TSStep(ts);CHKERRQ(ierr);

	/* Get the current time and solution */
	ierr = TSGetTime(ts,&t);CHKERRQ(ierr);
	ierr = TSGetSolution(ts,&U);CHKERRQ(ierr);
	ierr = TSGetTimeStepNumber(ts,&stepnum);CHKERRQ(ierr);
	ierr = TSMonitor(ts,stepnum,t,U);CHKERRQ(ierr);
	
	/* Check for event zero */
	ierr = (*event->monitor)(ts,t,U,event->fvalue,event->direction,event->terminate,NULL);CHKERRQ(ierr);
      }
      if (event->terminate[i]) {
	ierr = PetscPrintf(PETSC_COMM_SELF,"Ball height = %f\n",event->fvalue[i]);CHKERRQ(ierr);
	ierr = TSSetConvergedReason(ts,TS_CONVERGED_USER);CHKERRQ(ierr);
      }
    }
    event->fvalue_prev[i] = event->fvalue[i];
    event->ptime_prev  = t;

  }
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "IFunction"
/*
     Defines the ODE passed to the ODE solver
*/
static PetscErrorCode IFunction(TS ts,PetscReal t,Vec U,Vec Udot,Vec F,void *ctx)
{
  PetscErrorCode ierr;
  PetscScalar    *u,*udot,*f;

  PetscFunctionBegin;
  /*  The next three lines allow us to access the entries of the vectors directly */
  ierr = VecGetArray(U,&u);CHKERRQ(ierr);
  ierr = VecGetArray(Udot,&udot);CHKERRQ(ierr);
  ierr = VecGetArray(F,&f);CHKERRQ(ierr);

  f[0] = udot[0] - u[1];
  f[1] = udot[1] + 9.8;

  ierr = VecRestoreArray(U,&u);CHKERRQ(ierr);
  ierr = VecRestoreArray(Udot,&udot);CHKERRQ(ierr);
  ierr = VecRestoreArray(F,&f);CHKERRQ(ierr);
  PetscFunctionReturn(0);
}

#undef __FUNCT__
#define __FUNCT__ "IJacobian"
/*
     Defines the Jacobian of the ODE passed to the ODE solver. See TSSetIJacobian() for the meaning of a and the Jacobian.
*/
static PetscErrorCode IJacobian(TS ts,PetscReal t,Vec U,Vec Udot,PetscReal a,Mat *A,Mat *B,MatStructure *flag,void *ctx)
{
  PetscErrorCode ierr;
  PetscInt       rowcol[] = {0,1};
  PetscScalar    *u,*udot,J[2][2];

  PetscFunctionBegin;
  ierr = VecGetArray(U,&u);CHKERRQ(ierr);
  ierr = VecGetArray(Udot,&udot);CHKERRQ(ierr);

  J[0][0] = a;                       J[0][1] = -1;
  J[1][0] = 0.0;                     J[1][1] = a;
  ierr    = MatSetValues(*B,2,rowcol,2,rowcol,&J[0][0],INSERT_VALUES);CHKERRQ(ierr);

  ierr    = VecRestoreArray(U,&u);CHKERRQ(ierr);
  ierr    = VecRestoreArray(Udot,&udot);CHKERRQ(ierr);

  ierr = MatAssemblyBegin(*A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  ierr = MatAssemblyEnd(*A,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  if (*A != *B) {
    ierr = MatAssemblyBegin(*B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
    ierr = MatAssemblyEnd(*B,MAT_FINAL_ASSEMBLY);CHKERRQ(ierr);
  }
  *flag = SAME_NONZERO_PATTERN;
  PetscFunctionReturn(0);
}
  
#undef __FUNCT__
#define __FUNCT__ "main"
int main(int argc,char **argv)
{
  TS             ts;            /* ODE integrator */
  Vec            U;             /* solution will be stored here */
  Mat            A;             /* Jacobian matrix */
  PetscErrorCode ierr;
  PetscMPIInt    size;
  PetscInt       n = 2,nruns=10,i;
  PetscScalar    *u;

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Initialize program
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = PetscInitialize(&argc,&argv,(char*)0,help);CHKERRQ(ierr);
  ierr = MPI_Comm_size(PETSC_COMM_WORLD,&size);CHKERRQ(ierr);
  if (size > 1) SETERRQ(PETSC_COMM_WORLD,PETSC_ERR_SUP,"Only for sequential runs");

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    Create necessary matrix and vectors
    - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = MatCreate(PETSC_COMM_WORLD,&A);CHKERRQ(ierr);
  ierr = MatSetSizes(A,n,n,PETSC_DETERMINE,PETSC_DETERMINE);CHKERRQ(ierr);
  ierr = MatSetType(A,MATDENSE);CHKERRQ(ierr);
  ierr = MatSetFromOptions(A);CHKERRQ(ierr);
  ierr = MatSetUp(A);CHKERRQ(ierr);

  ierr = MatGetVecs(A,&U,NULL);CHKERRQ(ierr);

  ierr = VecGetArray(U,&u);CHKERRQ(ierr);
  u[0] = 0.0;
  u[1] = 20.0;
  ierr = VecRestoreArray(U,&u);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Create timestepping solver context
     - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = TSCreate(PETSC_COMM_WORLD,&ts);CHKERRQ(ierr);
  ierr = TSSetProblemType(ts,TS_NONLINEAR);CHKERRQ(ierr);
  ierr = TSSetType(ts,TSROSW);CHKERRQ(ierr);
  ierr = TSSetIFunction(ts,NULL,(TSIFunction) IFunction,NULL);CHKERRQ(ierr);
  ierr = TSSetIJacobian(ts,A,A,(TSIJacobian)IJacobian,NULL);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set initial conditions
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = TSSetSolution(ts,U);CHKERRQ(ierr);

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Set solver options
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = TSSetDuration(ts,1000,50.0);CHKERRQ(ierr);
  ierr = TSSetInitialTimeStep(ts,0.0,0.1);CHKERRQ(ierr);
  ierr = TSSetFromOptions(ts);CHKERRQ(ierr);
  ierr = TSSetPostStep(ts,PostStep);CHKERRQ(ierr);
  
  ierr = EventMonitorSet(ts,1,EventFunction,NULL);CHKERRQ(ierr);

  for (i=0; i < nruns; i++) {
    PetscReal t;
    /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
       Run timestepping solver
       - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
    ierr = TSSolve(ts,U);CHKERRQ(ierr);

    ierr = TSGetTime(ts,&t);CHKERRQ(ierr);
    ierr = TSGetSolution(ts,&U);CHKERRQ(ierr);

    /* Set new initial conditions with .9 attenuation */
    ierr = VecGetArray(U,&u);CHKERRQ(ierr);
    u[0] = 0.0;
    u[1] = -0.9*u[1];
    ierr = VecRestoreArray(U,&u);CHKERRQ(ierr);

    ierr = TSSetSolution(ts,U);CHKERRQ(ierr);
    ierr = TSSetInitialTimeStep(ts,t,0.1);CHKERRQ(ierr);
    ierr = TSSetDuration(ts,1000,50.0);CHKERRQ(ierr);

  }

  /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
     Free work space.  All PETSc objects should be destroyed when they are no longer needed.
   - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
  ierr = MatDestroy(&A);CHKERRQ(ierr);
  ierr = VecDestroy(&U);CHKERRQ(ierr);
  ierr = TSDestroy(&ts);CHKERRQ(ierr);

  ierr = PetscFinalize();
  return(0);
}
