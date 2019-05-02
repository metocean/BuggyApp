kubectl apply -f buggyapp-lb.yaml
kubectl apply -f buggyapp-deployment.yaml
kubectl port-forward svc/buggyapp-lb 8080:8080
#while true; do clear; kubectl get all; sleep 3; done

#cd BuggyApp
#compile
git submodule update --init --recursive

docker run --rm -v "$(pwd)":/BuggyApp -w /BuggyApp gcc:8.3 /bin/bash -c "make clean; make"
#build
docker build --tag=sergeimelman/buggyapp .
#push
docker push sergeimelman/buggyapp
#test
docker run --rm --name=buggyapp -p 8080:8080 sergeimelman/buggyapp 
#kill
docker kill buggyapp
