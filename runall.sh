kubectl apply -f buggyapp-deployment.yaml --record



##tests
#kubectl port-forward svc/buggyapp-lb 8080:8080
while true; do date >txt;kubectl get ing,all >>txt 2>&1; echo ----- >>txt;kubectl top pods>>txt 2>&1; clear; cat txt; sleep 3; done
kubectl logs -f -l app=buggyapp

kubectl rollout history deployment
kubectl rollout history all

##build multystage build process. build and push 
BAV=1; docker build --rm --tag=sergeimelman/buggyapp:$BAV .;docker push sergeimelman/buggyapp:$BAV

#cd BuggyApp
##compile
git clone --recursive git@github.com:metocean/BuggyApp.git
git submodule update --init --recursive
git submodule foreach git checkout origin/master

##compile 
## DEPRECATED docker run --rm -v "$(pwd)":/BuggyApp -w /BuggyApp gcc:8.3 /bin/bash -c "make clean; make"
## DEPRECATED docker build --rm -f Dockerfile.a --tag=alpine_build .
## DEPRECATED docker run --rm -v "$(pwd)":/BuggyApp -w /BuggyApp alpine_build /bin/sh -c "make clean; make; make clean"


##test
docker run --rm --name=buggyapp -p 8080:8080 sergeimelman/buggyapp
docker run --rm --name=buggyapp -p 8080:8080 -ti sergeimelman/buggyapp /bin/sh
##kill
docker kill buggyapp

byggyapp.rag.metocean.co.nz