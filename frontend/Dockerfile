# Multi-stage: build the static SPA, then serve it with nginx.
FROM node:20-alpine AS build
WORKDIR /app
COPY package.json package-lock.json* ./
RUN npm ci || npm install
COPY . .
# VITE_* are baked in at build time, so they must be present here (build args).
ARG VITE_API_BASE
ARG VITE_WS_BASE
ARG VITE_API_KEY
RUN npm run build

FROM nginx:alpine
COPY nginx.conf /etc/nginx/conf.d/default.conf
COPY --from=build /app/dist /usr/share/nginx/html
EXPOSE 80
CMD ["nginx", "-g", "daemon off;"]
